/****************************************************************************
** Meta object code from reading C++ file 'votingdialog.h'
**
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "qt/votingdialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'votingdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.5.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_VotingDialog_t {
    QByteArrayData data[5];
    char stringdata0[106];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VotingDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VotingDialog_t qt_meta_stringdata_VotingDialog = {
    {
QT_MOC_LITERAL(0, 0, 12), // "VotingDialog"
QT_MOC_LITERAL(1, 13, 31), // "on_button_ViewProposals_clicked"
QT_MOC_LITERAL(2, 45, 0), // ""
QT_MOC_LITERAL(3, 46, 32), // "on_button_CreateProposal_clicked"
QT_MOC_LITERAL(4, 79, 26) // "on_button_SetVotes_clicked"

    },
    "VotingDialog\0on_button_ViewProposals_clicked\0"
    "\0on_button_CreateProposal_clicked\0"
    "on_button_SetVotes_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VotingDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x0a /* Public */,
       3,    0,   30,    2, 0x0a /* Public */,
       4,    0,   31,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void VotingDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        VotingDialog *_t = static_cast<VotingDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->on_button_ViewProposals_clicked(); break;
        case 1: _t->on_button_CreateProposal_clicked(); break;
        case 2: _t->on_button_SetVotes_clicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject VotingDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_VotingDialog.data,
      qt_meta_data_VotingDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *VotingDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VotingDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_VotingDialog.stringdata0))
        return static_cast<void*>(const_cast< VotingDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int VotingDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
