/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
 * See AUTHORS file for a full list of contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
