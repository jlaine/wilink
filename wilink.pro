include(wilink.pri)

TEMPLATE = subdirs

SUBDIRS = src

# Package generation
mac {
    package.depends = first
    package.output = wiLink-$$WILINK_VERSION-mac.dmg
    package.tmp = wiLink.tmp
    package.commands =  \
        rm -rf $$package.tmp $$package.output; \
        $(MKDIR) $$package.tmp; \
        ln -s /Applications $$package.tmp/Applications; \
        cp -a $$WILINK_APP_PATH/wiLink.app $$package.tmp; \
        hdiutil create $$package.output -srcdir $$package.tmp -format UDBZ -volname \"wiLink $$WILINK_VERSION\"; \
        rm -rf $$package.tmp
    QMAKE_EXTRA_TARGETS = package
} else:win32 {
    package.depends = first
    package.output = wiLink-$$WILINK_VERSION-win32.exe
    package.commands = makensis wilink.nsi
    QMAKE_EXTRA_TARGETS = package

    NSI_HEADER = "!define PRODUCT_VERSION \"$$WILINK_VERSION\""
    NSI_HEADER += "!define PRODUCT_LICENSE \"$$system_path($$WILINK_SOURCE_TREE/COPYING)\""
    NSI_HEADER += "!define PRODUCT_OUTPUT \"$$system_path($$package.output)\""
    NSI_BODY = $$cat($$WILINK_SOURCE_TREE/wilink.nsi.in, blob)
    write_file($$WILINK_BUILD_TREE/wilink.nsi, NSI_HEADER)
    write_file($$WILINK_BUILD_TREE/wilink.nsi, NSI_BODY, append)
    QMAKE_CLEAN += wilink.nsi
}

OTHER_FILES += \
    android/AndroidManifest.xml \
    android/res/drawable/icon.png \
    android/res/drawable/logo.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    android/res/layout/splash.xml \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-fr/strings.xml \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/kde/necessitas/origo/QtActivity.java \
    android/src/org/kde/necessitas/origo/QtApplication.java \
    android/version.xml

