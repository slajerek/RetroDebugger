-- VICE stub launcher (outer .app)
-- Expects VICE.app to be in the same folder as this stub app.

on run
  set stubPath to POSIX path of (path to me)
  set (viceAppPath, programName) to resolveViceAndProgram(stubPath)
  launchVice(viceAppPath, programName, {})
end run

on open theItems
  set stubPath to POSIX path of (path to me)
  set (viceAppPath, programName) to resolveViceAndProgram(stubPath)
  launchVice(viceAppPath, programName, theItems)
end open

on resolveViceAndProgram(stubPath)
  -- stubPath: /Some/Folder/x64sc.app
  set stubDir to do shell script "dirname " & quoted form of stubPath
  set viceAppPath to stubDir & "/VICE.app"

  -- derive program name from stub bundle name
  set programName to do shell script "basename " & quoted form of stubPath & " .app"

  -- check for VICE.app
  try
    do shell script "test -d " & quoted form of viceAppPath
  on error
    display dialog programName & ".app requires VICE.app to be installed in the same folder.\n\nExpected:\n" & viceAppPath buttons {"OK"} default button 1 with icon stop
    error number -128 -- user cancelled / abort
  end try

  return {viceAppPath, programName}
end resolveViceAndProgram

on launchVice(viceAppPath, programName, theItems)
  -- Build quoted file args
  set fileArgs to ""
  repeat with f in theItems
    set fileArgs to fileArgs & " " & quoted form of POSIX path of f
  end repeat

  -- IMPORTANT:
  -- This passes the emulator selection as an argv pair: --program <name>
  -- Your VICE.app launcher script must parse this (example below).
  set cmd to "open " & quoted form of viceAppPath & " --args --program " & quoted form of programName & fileArgs
  do shell script cmd
end launchVice