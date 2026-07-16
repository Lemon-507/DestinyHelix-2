TEMPLATE = subdirs

SUBDIRS = \
    corelib \
    app \
    tests

app.depends = corelib
tests.depends = corelib
