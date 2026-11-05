import assert from 'node:assert/strict';
import { BulkReceiver, ackFrame, crc32, downloadBulk } from '../app-handoff/bulkDownload.mjs';

const wav = new Uint8Array(640044);
const view = new DataView(wav.buffer);
for (const [at, text] of [[0,'RIFF'],[8,'WAVE'],[12,'fmt '],[36,'data']])
  for (let i = 0; i < text.length; ++i) wav[at+i] = text.charCodeAt(i);
view.setUint32(4, wav.length-8, true); view.setUint32(16,16,true);
view.setUint16(20,1,true); view.setUint16(22,1,true); view.setUint32(24,16000,true);
view.setUint32(28,32000,true); view.setUint16(32,2,true); view.setUint16(34,16,true);
view.setUint32(40,640000,true);
for (let i = 44; i < wav.length; ++i) wav[i] = i * 31;
const manifest = { jobId:'j1', captureId:'c1', fileId:'f1', sensorId:1, sampleCount:320000,
  byteLength:wav.length, crc32:((crc32(wav)^0xffffffff)>>>0).toString(16).padStart(8,'0') };
const open = mtu => ({ ...manifest, bootId:'b1', transferId:7, mtu,
  valueBytes:Math.min(mtu-3,512), payloadBytes:Math.min(mtu-3,512)-20 });
function packet(offset, count, transfer=7) {
  const b = new Uint8Array(20+count), v = new DataView(b.buffer);
  b.set([65,66,87,49]); v.setUint32(4,transfer,true); v.setUint32(8,offset,true);
  b[12]=1; b[13]=offset+count===wav.length?1:0; v.setUint16(14,count,true);
  b.set(wav.subarray(offset,offset+count),20); return b;
}
for (const mtu of [179,185,247,517]) {
  const rx = new BulkReceiver(open(mtu),manifest,'b1');
  while (rx.next < wav.length) {
    const result = rx.accept(packet(rx.next,Math.min(open(mtu).payloadBytes,wav.length-rx.next)));
    if (result.acknowledge) rx.grant();
  }
  assert.deepEqual(rx.bytes,wav);
}
assert.throws(()=>new BulkReceiver({...open(185),sensorId:2},manifest,'b1'),/manifest/);
assert.throws(()=>new BulkReceiver(open(185),manifest,'old'),/Stale/);
const rx = new BulkReceiver(open(185),manifest,'b1');
assert.equal(rx.accept(packet(0,162,99)),null);
assert.throws(()=>rx.accept(packet(162,162)),/gap/);
rx.accept(packet(0,162));
assert.throws(()=>rx.accept(packet(0,162)),/gap/);
const wrongSensor = packet(162,162); wrongSensor[12]=2;
assert.throws(()=>rx.accept(wrongSensor),/identity/);
// Identity must match OPEN before CRC is checked at EOF.
const corruptOpen = {...open(185),crc32:'00000000'};
const corrupt = new BulkReceiver(corruptOpen,{...manifest,crc32:'00000000'},'b1');
assert.throws(()=>{
  while(corrupt.next<wav.length){ const e=corrupt.accept(packet(corrupt.next,Math.min(162,wav.length-corrupt.next))); if(e.acknowledge)corrupt.grant(); }
},/CRC/);
assert.deepEqual([...ackFrame(7,0,16)],[65,66,75,49,7,0,0,0,0,0,0,0,16,0,0,0]);

// Exercise the real async orchestrator with native-like ACKs and window stops.
let callback, offset=0, validated=false, closed=false, cancelled=false, statusCalls=0;
const link = {
  async subscribe(uuid,onBytes){ callback=onBytes; return ()=>{closed=true;}; },
  async command(cmd,fields){
    if(cmd==='BULK_OPEN')return {...open(185),state:'awaiting_ack'};
    if(cmd==='BULK_CANCEL'){cancelled=true;return {};}
    assert.equal(cmd,'BULK_STATUS'); ++statusCalls;
    return {...open(185),state:'complete',acknowledgedBytes:offset,memoryRetries:0};
  },
  async write(uuid,bytes){
    const v=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);
    assert.equal(v.getUint32(8,true),offset);
    const grant=v.getUint16(12,true);
    // A response may arrive before the native ACK write Promise resolves.
    for(let i=0;i<grant && offset<wav.length;++i){
      const count=Math.min(162,wav.length-offset), p=packet(offset,count); offset+=count; callback(p);
    }
  }
};
const downloaded = await downloadBulk(link,{bootId:'b1',bulkDownload:{version:1,ackTimeoutMs:10000}},manifest,{
  validateWav(bytes,m){
    assert.equal(bytes.length,640044); const v=new DataView(bytes.buffer);
    assert.equal(v.getUint32(24,true),16000); assert.equal(v.getUint32(40,true),m.sampleCount*2);
    assert.deepEqual(bytes,wav); validated=true;
  }
});
assert.deepEqual(downloaded.bytes,wav);
assert.ok(validated && closed && !cancelled && statusCalls===1);
// A stalled window must release the transfer before the caller uses READ_FILE.
let fallbackCancelled=false, unsubscribed=false;
const stalled={
  async subscribe(){return ()=>{unsubscribed=true;};},
  async write(){},
  async command(cmd){
    if(cmd==='BULK_OPEN')return open(185);
    if(cmd==='BULK_CANCEL'){fallbackCancelled=true;return {};}
    if(cmd==='READ_FILE'){assert.ok(fallbackCancelled);return {data:'fallback'};}
    throw new Error('Unexpected command');
  }
};
await assert.rejects(downloadBulk(stalled,{bootId:'b1',bulkDownload:{version:1,ackTimeoutMs:10}},manifest,
  {validateWav(){throw new Error('Unverified data must never reach validation');}}),/stalled/);
assert.ok(fallbackCancelled && unsubscribed);
assert.equal((await stalled.command('READ_FILE')).data,'fallback');
console.log('PASS: original 640044-byte WAV, MTU limits, identities, missing/duplicate data, CRC, async credit windows and final confirmation.');
