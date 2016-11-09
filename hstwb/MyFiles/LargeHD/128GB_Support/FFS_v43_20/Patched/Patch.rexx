/* rexx */

infile  = "RAM:FFS43_20/FastFileSystem"
outfile = "RAM:FastFileSystem"

offset  = x2d(2A34)
olddata = '610002E6'x
newdata = '4E714E71'x

if ~Open(in,infile,read) then do
    say "patch failed: cannot open input file"
    exit 20
end


buffer = ReadCh(in,65535)
call Close(in)

if substr(buffer,offset+1,4) ~= olddata then do
    say "patch failed: unexpected data at the place to patch"
    say "expected" c2x(olddata) "found" c2x(substr(buffer,offset+1,4))
    exit 20
end

if ~Open(out,outfile,write) then do
    say "patch failed: cannot open output file"
    exit 20
end

call WriteCh out,substr(buffer,1,offset)||newdata||substr(buffer,offset+5)

call Close(out)

say "patch successful"