#include <Carbon/Carbon.h>
#include <QString>

bool qt_mac_execute_apple_script(const QString &scr)
{
    const QByteArray l = scr.toUtf8();
    const char *script = l.constData();
    long script_len = l.size();

    OSStatus err;
    AEDesc scriptTextDesc;
    ComponentInstance theComponent = 0;
    OSAID scriptID = kOSANullScript, resultID = kOSANullScript;

    // set up locals to a known state
    AECreateDesc(typeNull, 0, 0, &scriptTextDesc);
    scriptID = kOSANullScript;
    resultID = kOSANullScript;

    // open the scripting component
    theComponent = OpenDefaultComponent(kOSAComponentType, typeAppleScript);
    if (!theComponent) {
        err = paramErr;
        goto bail;
    }

    // put the script text into an aedesc
    err = AECreateDesc(typeUTF8Text, script, script_len, &scriptTextDesc);
    if (err != noErr)
        goto bail;

    // compile the script
    err = OSACompile(theComponent, &scriptTextDesc, kOSAModeNull, &scriptID);
    if (err != noErr)
        goto bail;

    // run the script
    err = OSAExecute(theComponent, scriptID, kOSANullScript, kOSAModeNull, &resultID);
bail:
    AEDisposeDesc(&scriptTextDesc);
    if (scriptID != kOSANullScript)
        OSADispose(theComponent, scriptID);
    if (resultID != kOSANullScript)
        OSADispose(theComponent, resultID);
    if (theComponent)
        CloseComponent(theComponent);
    return err == noErr;
}
