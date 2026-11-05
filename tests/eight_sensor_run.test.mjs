import assert from 'node:assert/strict';
import { crc32 } from '../app-handoff/bulkDownload.mjs';
import { runEightSensorBenchmark, verifyScanManifest } from '../app-handoff/eightSensorRun.mjs';
const wavs = new Map();
const files = Array.from({length:8},(_,index)=>{
  const sensorId=index+1, bytes=new Uint8Array(640044), view=new DataView(bytes.buffer);
  for(const [at,text] of [[0,'RIFF'],[8,'WAVE'],[12,'fmt '],[36,'data']])
    for(let i=0;i<text.length;++i)bytes[at+i]=text.charCodeAt(i);
  view.setUint32(4,640036,true); view.setUint32(16,16,true);
  view.setUint16(20,1,true); view.setUint16(22,1,true); view.setUint32(24,16000,true);
  view.setUint32(28,32000,true); view.setUint16(32,2,true); view.setUint16(34,16,true);
  view.setUint32(40,640000,true); bytes.fill(sensorId,44);
  wavs.set(`f${sensorId}`,bytes);
  return {jobId:'j1',captureId:`c${sensorId}`,fileId:`f${sensorId}`,sensorId,sampleCount:320000,
    byteLength:bytes.length,crc32:((crc32(bytes)^0xffffffff)>>>0).toString(16).padStart(8,'0')};
});
const job={bootId:'b1',jobId:'j1',phase:'complete',files};
assert.equal(verifyScanManifest(job,'b1','j1').length,8);
for(const change of [f=>{f[1].sensorId=1;},f=>{f[1].fileId=f[0].fileId;},
  f=>{f[1].captureId=f[0].captureId;},f=>{f[2].byteLength--;},f=>{f[3].crc32='invalid';},
  f=>{delete f[0].captureId;},f=>{delete f[0].fileId;}]) {
  const copy=structuredClone(job); change(copy.files);
  assert.throws(()=>verifyScanManifest(copy,'b1','j1'),/Invalid/);
}
const caps={bootId:'b1',ready:true,modes:['single','scan'],diagnostics:{build:'v2-bulk-timing-3'},
  bulkDownload:{version:1,ackTimeoutMs:10000}};
function makeLink(failSensor=0){
  let receive,onError,current,offset=0,handle=0,failed=false,active=false;
  const starts=[],opens=[];
  return {starts,opens,
    async subscribe(uuid,cb,err){receive=cb;onError=err;return ()=>{};},
    async command(cmd,fields){
      if(cmd==='START_SCAN'){starts.push(cmd);return {bootId:'b1',jobId:'j1'};}
      if(cmd==='GET_JOB')return job;
      if(cmd==='BULK_OPEN'){
        assert.equal(active,false);current=files.find(f=>f.fileId===fields.fileId);
        opens.push(current.sensorId);offset=0;failed=false;active=true;++handle;
        return {...current,bootId:'b1',transferId:handle,mtu:517,valueBytes:512,payloadBytes:492};
      }
      if(cmd==='BULK_CANCEL'){active=false;failed=true;}
      assert.ok(cmd==='BULK_STATUS'||cmd==='BULK_CANCEL');
      return {...current,bootId:'b1',transferId:handle,state:failed?'failed':'complete',
        error:failed?'SIMULATED_TX_ERROR':'',acknowledgedBytes:offset,timing:{lastSubmitCode:failed?6:0}};
    },
    async write(uuid,ack){
      const a=new DataView(ack.buffer);assert.equal(a.getUint32(8,true),offset);
      if(current.sensorId===failSensor){onError(new Error('Simulated transfer failure'));return;}
      if(offset===current.byteLength){active=false;return;}
      for(let i=0;i<a.getUint16(12,true)&&offset<current.byteLength;++i){
        const count=Math.min(492,current.byteLength-offset),p=new Uint8Array(20+count),v=new DataView(p.buffer);
        p.set([65,66,87,49]);v.setUint32(4,handle,true);v.setUint32(8,offset,true);
        p[12]=current.sensorId;p[13]=offset+count===current.byteLength?1:0;v.setUint16(14,count,true);
        p.set(wavs.get(current.fileId).subarray(offset,offset+count),20);offset+=count;receive(p);
      }
    }
  };
}
const options={validateWav(bytes,file){
  assert.deepEqual(bytes,wavs.get(file.fileId));assert.equal(new DataView(bytes.buffer).getUint32(24,true),16000);
},async persistAndReadback(bytes,file){
  const stored=bytes.slice();assert.deepEqual(stored,wavs.get(file.fileId));return `local://${file.fileId}.wav`;
}};
const link=makeLink(), report=await runEightSensorBenchmark(link,caps,options);
assert.equal(report.state,'complete');assert.equal(report.files.length,8);
assert.equal(link.starts.length,1);assert.deepEqual(link.opens,[1,2,3,4,5,6,7,8]);
assert.ok(report.allPlayableFromDownloadMs>=report.firstPlayableFromDownloadMs);
assert.ok(report.allPlayableFromCaptureMs>=report.allPlayableFromDownloadMs);
const failing=makeLink(3), failure=await runEightSensorBenchmark(failing,caps,options);
assert.equal(failure.state,'failed');assert.equal(failure.files.length,2);
assert.deepEqual(failing.opens,[1,2,3]);assert.equal(failure.failure.sensorId,3);
assert.equal(failure.failure.nextOffset,0);assert.equal(failure.failure.terminalBulkStatus.state,'failed');
assert.equal(failure.failure.timing.lastSubmitCode,6);
console.log('PASS: one START_SCAN, eight distinct verified manifests, sequential original WAVs, readback, first/all availability, and sensor/offset/terminal timing failure report. Host simulation only.');
