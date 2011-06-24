TEMPLATE = app
TARGET = qm_phony_target

TRANSLATIONS = fr.ts qt_fr.ts
QRC = translations.qrc.cmake

qtPrepareTool(LRELEASE, lrelease)

updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_BASE}.qm
updateqm.variable_out = PRE_TARGETDEPS
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

copyqrc.input = QRC
copyqrc.output = translations.qrc
copyqrc.variable_out = PRE_TARGETDEPS
copyqrc.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copyqrc.name = COPY ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += copyqrc

QMAKE_LINK = @: IGNORE THIS LINE
OBJECTS_DIR =
win32:CONFIG -= embed_manifest_exe
