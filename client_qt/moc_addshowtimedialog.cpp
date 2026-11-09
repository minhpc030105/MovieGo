/****************************************************************************
** Meta object code from reading C++ file 'addshowtimedialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "addshowtimedialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'addshowtimedialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AddShowTimeDialog_t {
    QByteArrayData data[8];
    char stringdata0[110];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AddShowTimeDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AddShowTimeDialog_t qt_meta_stringdata_AddShowTimeDialog = {
    {
QT_MOC_LITERAL(0, 0, 17), // "AddShowTimeDialog"
QT_MOC_LITERAL(1, 18, 14), // "onFilmSelected"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 5), // "index"
QT_MOC_LITERAL(4, 40, 16), // "onCinemaSelected"
QT_MOC_LITERAL(5, 57, 14), // "onRoomSelected"
QT_MOC_LITERAL(6, 72, 20), // "onAddShowTimeClicked"
QT_MOC_LITERAL(7, 93, 16) // "onRefreshClicked"

    },
    "AddShowTimeDialog\0onFilmSelected\0\0"
    "index\0onCinemaSelected\0onRoomSelected\0"
    "onAddShowTimeClicked\0onRefreshClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AddShowTimeDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x08 /* Private */,
       4,    1,   42,    2, 0x08 /* Private */,
       5,    1,   45,    2, 0x08 /* Private */,
       6,    0,   48,    2, 0x08 /* Private */,
       7,    0,   49,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void AddShowTimeDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AddShowTimeDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onFilmSelected((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->onCinemaSelected((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->onRoomSelected((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onAddShowTimeClicked(); break;
        case 4: _t->onRefreshClicked(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AddShowTimeDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_AddShowTimeDialog.data,
    qt_meta_data_AddShowTimeDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AddShowTimeDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AddShowTimeDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AddShowTimeDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int AddShowTimeDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
