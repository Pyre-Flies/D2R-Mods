"""Read-only controller normalized-set monitor, bounded to 55 seconds."""
exec(open(__file__.replace('monitor_native_controller.py','audit_native_controller.py')).read().split('try:\n for name')[0])
import time,json
out=[];last=None
try:
 for _ in range(1100):
  idx=struct.unpack('<I',read(game+0x2a23704,4))[0]
  rows=[]
  for n in range(8):
   a=game+0x2a4dc20+n*0x1c8
   count,table=struct.unpack('<QQ',read(a,16));keys=[]
   if 0<count<=256 and table:
    for head in struct.unpack('<'+'Q'*count,read(table,count*8)):
     seen=set()
     while head and len(seen)<32 and head not in seen:
      seen.add(head);nextp,key=struct.unpack('<QI',read(head,12));keys.append(hex(key));head=nextp
   if keys:rows.append((n,sorted(keys)))
  state=(idx,rows)
  if state!=last:
   rec={'time':time.strftime('%H:%M:%S'),'state':state};out.append(rec);print(json.dumps(rec),flush=True);last=state
  time.sleep(.05)
finally:
 k.CloseHandle(h)
 Path(__file__).with_name('native-controller-observation-'+time.strftime('%Y%m%d-%H%M%S')+'.json').write_text(json.dumps(out,indent=2))
