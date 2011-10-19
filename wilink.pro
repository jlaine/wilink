include(wilink.pri)

TEMPLATE = subdirs

SUBDIRS = src

# Package generation
PACKAGE = wiLink
VERSION = $$WILINK_VERSION
mac {
    package.output = $$PACKAGE-$$VERSION-mac-10.6.dmg
    package.bundle = $${PACKAGE}.app
    package.tmp = $${PACKAGE}.tmp
    package.commands =  \
        rm -rf $$package.tmp $$package.output; \
        mkdir $$package.tmp; \
        ln -s /Applications $$package.tmp/Applications; \
        cp -r src/plugins/$$package.bundle $$package.tmp; \
        $$[QT_INSTALL_BINS]/macdeployqt $$package.tmp/$$package.bundle; \
        cmake/copyplugins $$package.tmp/$$package.bundle $$QMAKE_QMAKE \
            $$[QT_INSTALL_PLUGINS]/imageformats/libqgif.* \
            $$[QT_INSTALL_PLUGINS]/imageformats/libqjpeg.* \
            $$[QT_INSTALL_PLUGINS]/sqldrivers/libqsqlite.*; \
        hdiutil create $$package.output -srcdir $$package.tmp -fs HFS+ -volname \"$$PACKAGE $$VERSION\"; \
        rm -rf $$package.tmp
    QMAKE_EXTRA_TARGETS = package
}
