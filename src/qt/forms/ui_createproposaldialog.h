/********************************************************************************
** Form generated from reading UI file 'createproposaldialog.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CREATEPROPOSALDIALOG_H
#define UI_CREATEPROPOSALDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CreateProposalDialog
{
public:
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label_ProposalName;
    QLineEdit *lineEdit_Name;
    QLabel *label_Abstract;
    QLineEdit *lineEdit_Abstract;
    QLabel *label_StartBlock;
    QLineEdit *lineEdit_StartBlock;
    QLabel *label_Length;
    QLineEdit *lineEdit_Length;
    QLabel *label_Hash;
    QLabel *label_Hash_result;
    QLabel *label_Location;
    QLabel *label_Location_result;
    QLabel *label_Size;
    QLabel *label_Size_result;
    QLabel *label_Fee;
    QLabel *label_Fee_result;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *button_CreateProposal;
    QPushButton *button_SendProposal;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CreateProposalDialog)
    {
        if (CreateProposalDialog->objectName().isEmpty())
            CreateProposalDialog->setObjectName(QStringLiteral("CreateProposalDialog"));
        CreateProposalDialog->resize(676, 308);
        verticalLayoutWidget = new QWidget(CreateProposalDialog);
        verticalLayoutWidget->setObjectName(QStringLiteral("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(10, 10, 641, 278));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        formLayout = new QFormLayout();
        formLayout->setObjectName(QStringLiteral("formLayout"));
        label_ProposalName = new QLabel(verticalLayoutWidget);
        label_ProposalName->setObjectName(QStringLiteral("label_ProposalName"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label_ProposalName);

        lineEdit_Name = new QLineEdit(verticalLayoutWidget);
        lineEdit_Name->setObjectName(QStringLiteral("lineEdit_Name"));

        formLayout->setWidget(0, QFormLayout::FieldRole, lineEdit_Name);

        label_Abstract = new QLabel(verticalLayoutWidget);
        label_Abstract->setObjectName(QStringLiteral("label_Abstract"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_Abstract);

        lineEdit_Abstract = new QLineEdit(verticalLayoutWidget);
        lineEdit_Abstract->setObjectName(QStringLiteral("lineEdit_Abstract"));

        formLayout->setWidget(1, QFormLayout::FieldRole, lineEdit_Abstract);

        label_StartBlock = new QLabel(verticalLayoutWidget);
        label_StartBlock->setObjectName(QStringLiteral("label_StartBlock"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_StartBlock);

        lineEdit_StartBlock = new QLineEdit(verticalLayoutWidget);
        lineEdit_StartBlock->setObjectName(QStringLiteral("lineEdit_StartBlock"));

        formLayout->setWidget(2, QFormLayout::FieldRole, lineEdit_StartBlock);

        label_Length = new QLabel(verticalLayoutWidget);
        label_Length->setObjectName(QStringLiteral("label_Length"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_Length);

        lineEdit_Length = new QLineEdit(verticalLayoutWidget);
        lineEdit_Length->setObjectName(QStringLiteral("lineEdit_Length"));

        formLayout->setWidget(3, QFormLayout::FieldRole, lineEdit_Length);

        label_Hash = new QLabel(verticalLayoutWidget);
        label_Hash->setObjectName(QStringLiteral("label_Hash"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_Hash);

        label_Hash_result = new QLabel(verticalLayoutWidget);
        label_Hash_result->setObjectName(QStringLiteral("label_Hash_result"));

        formLayout->setWidget(4, QFormLayout::FieldRole, label_Hash_result);

        label_Location = new QLabel(verticalLayoutWidget);
        label_Location->setObjectName(QStringLiteral("label_Location"));

        formLayout->setWidget(5, QFormLayout::LabelRole, label_Location);

        label_Location_result = new QLabel(verticalLayoutWidget);
        label_Location_result->setObjectName(QStringLiteral("label_Location_result"));

        formLayout->setWidget(5, QFormLayout::FieldRole, label_Location_result);

        label_Size = new QLabel(verticalLayoutWidget);
        label_Size->setObjectName(QStringLiteral("label_Size"));

        formLayout->setWidget(6, QFormLayout::LabelRole, label_Size);

        label_Size_result = new QLabel(verticalLayoutWidget);
        label_Size_result->setObjectName(QStringLiteral("label_Size_result"));

        formLayout->setWidget(6, QFormLayout::FieldRole, label_Size_result);

        label_Fee = new QLabel(verticalLayoutWidget);
        label_Fee->setObjectName(QStringLiteral("label_Fee"));

        formLayout->setWidget(7, QFormLayout::LabelRole, label_Fee);

        label_Fee_result = new QLabel(verticalLayoutWidget);
        label_Fee_result->setObjectName(QStringLiteral("label_Fee_result"));

        formLayout->setWidget(7, QFormLayout::FieldRole, label_Fee_result);


        verticalLayout->addLayout(formLayout);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        button_CreateProposal = new QPushButton(verticalLayoutWidget);
        button_CreateProposal->setObjectName(QStringLiteral("button_CreateProposal"));

        horizontalLayout_3->addWidget(button_CreateProposal);

        button_SendProposal = new QPushButton(verticalLayoutWidget);
        button_SendProposal->setObjectName(QStringLiteral("button_SendProposal"));
        button_SendProposal->setEnabled(false);

        horizontalLayout_3->addWidget(button_SendProposal);


        verticalLayout->addLayout(horizontalLayout_3);

        buttonBox = new QDialogButtonBox(verticalLayoutWidget);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(CreateProposalDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), CreateProposalDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), CreateProposalDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(CreateProposalDialog);
    } // setupUi

    void retranslateUi(QDialog *CreateProposalDialog)
    {
        CreateProposalDialog->setWindowTitle(QApplication::translate("CreateProposalDialog", "Proposal Creation", 0));
        label_ProposalName->setText(QApplication::translate("CreateProposalDialog", "Proposal Name:", 0));
        label_Abstract->setText(QApplication::translate("CreateProposalDialog", "Proposal Abstract:", 0));
        label_StartBlock->setText(QApplication::translate("CreateProposalDialog", "Starting Block:", 0));
        label_Length->setText(QApplication::translate("CreateProposalDialog", "Voting Length (blocks):", 0));
        label_Hash->setText(QApplication::translate("CreateProposalDialog", "Proposal Hash:", 0));
        label_Hash_result->setText(QApplication::translate("CreateProposalDialog", "(Automatically Generated)", 0));
        label_Location->setText(QApplication::translate("CreateProposalDialog", "Bit Location:", 0));
        label_Location_result->setText(QApplication::translate("CreateProposalDialog", "(Automatically Generated)", 0));
        label_Size->setText(QApplication::translate("CreateProposalDialog", "Bit Size:", 0));
        label_Size_result->setText(QApplication::translate("CreateProposalDialog", "(Automatically Generated)", 0));
        label_Fee->setText(QApplication::translate("CreateProposalDialog", "Fee:", 0));
        label_Fee_result->setText(QApplication::translate("CreateProposalDialog", "(Automatically Generated)", 0));
        button_CreateProposal->setText(QApplication::translate("CreateProposalDialog", "Create Proposal", 0));
        button_SendProposal->setText(QApplication::translate("CreateProposalDialog", "Send Proposal", 0));
    } // retranslateUi

};

namespace Ui {
    class CreateProposalDialog: public Ui_CreateProposalDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CREATEPROPOSALDIALOG_H
