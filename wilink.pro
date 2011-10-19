include(wilink.pri)

TEMPLATE = subdirs

SUBDIRS = src

# Package generation
PACKAGE = wiLink
VERSION = $$WILINK_VERSION
mac {
    package.output = $$PACKAGE-$$VERSION-mac-10.6.dmg
    package.tmp = $${PACKAGE}.tmp
    package.commands =  \
        rm -rf $$package.tmp $$package.output; \
        mkdir $$package.tmp; \
        cp -r src/plugins/$${PACKAGE}.app $$package.tmp; \
        ln -s /Applications $$package.tmp/Applications; \
        hdiutil create $$package.output -srcdir $$package.tmp -fs HFS+ -volname \"$$PACKAGE $$VERSION\"; \
        rm -rf $$package.tmp
    QMAKE_EXTRA_TARGETS = package
}
