TEMPLATE = app
TARGET = qm_phony_target

TRANSLATIONS = fr.ts qt_fr.ts

qtPrepareTool(LRELEASE, lrelease)

updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_BASE}.qm
isEmpty(vcproj):updateqm.variable_out = PRE_TARGETDEPS
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
silent:updateqm.commands = @echo lrelease ${QMAKE_FILE_IN} && $$updateqm.commands
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

QMAKE_LINK = @: IGNORE THIS LINE
OBJECTS_DIR =
win32:CONFIG -= embed_manifest_exe
