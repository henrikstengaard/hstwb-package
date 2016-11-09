/* This is a magic script to add the correct NSDPatch lines to
 * the S:Startup-Sequence. It takes care to avoid multiple installations
 */

options results

ssname = "SYS:S/Startup-Sequence"

    /* We need it ... */
    if ~show('l', 'rexxsupport.library') then do
        call addlib('rexxsupport.library', 0, -30, 0)
    end

    foundnsd = 0
    call open(sfile, ssname, read)
    do while ~eof(sfile)
        line = readln(sfile)
        if line ~= '' then do
            if index(upper(line), "NSDPATCH") > 0 then do
                foundnsd = 1
            end
        end
    end
    call close(sfile)

    if foundnsd = 0 then do
        call open(sfile, ssname, read)
        call open(sofile, ssname".new", write)
        do while ~eof(sfile)
            line = readln(sfile)
            call writeln(sofile, line)
            if line ~= '' then do
                if index(upper(line), "SETPATCH") > 0 then do
                    foundnsd = 1
                    call writeln(sofile, "; BEGIN NSDPATCH")
                    call writeln(sofile, "IF EXISTS C:NSDPatch")
                    call writeln(sofile, "  C:NSDPatch QUIET")
                    call writeln(sofile, "ENDIF")
                    call writeln(sofile, "; END NSDPATCH")
                end
            end
        end
        call close(sofile)
        call close(sfile)

        if exists(ssname".beforensd") then do
            call delete(ssname".beforensd")
        end
        if exists(ssname) & exists(ssname".new") then do
            call rename(ssname, ssname".beforensd")
        end
        if exists (ssname".new") ~= 0 then do
            call rename(ssname".new", ssname)
        end
    end

exit 0
