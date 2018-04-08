/********************************************************************************
** Form generated from reading UI file 'votingdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VOTINGDIALOG_H
#define UI_VOTINGDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_VotingDialog
{
public:
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *button_ViewProposals;
    QPushButton *button_CreateProposal;
    QPushButton *button_SetVotes;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *VotingDialog)
    {
        if (VotingDialog->objectName().isEmpty())
            VotingDialog->setObjectName(QStringLiteral("VotingDialog"));
        VotingDialog->resize(194, 300);
        verticalLayoutWidget = new QWidget(VotingDialog);
        verticalLayoutWidget->setObjectName(QStringLiteral("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(10, 10, 171, 271));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        button_ViewProposals = new QPushButton(verticalLayoutWidget);
        button_ViewProposals->setObjectName(QStringLiteral("button_ViewProposals"));

        verticalLayout->addWidget(button_ViewProposals);

        button_CreateProposal = new QPushButton(verticalLayoutWidget);
        button_CreateProposal->setObjectName(QStringLiteral("button_CreateProposal"));

        verticalLayout->addWidget(button_CreateProposal);

        button_SetVotes = new QPushButton(verticalLayoutWidget);
        button_SetVotes->setObjectName(QStringLiteral("button_SetVotes"));

        verticalLayout->addWidget(button_SetVotes);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(verticalLayoutWidget);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(VotingDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), VotingDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), VotingDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(VotingDialog);
    } // setupUi

    void retranslateUi(QDialog *VotingDialog)
    {
        VotingDialog->setWindowTitle(QApplication::translate("VotingDialog", "HyperStake Voting System", 0));
        button_ViewProposals->setText(QApplication::translate("VotingDialog", "View Proposals", 0));
        button_CreateProposal->setText(QApplication::translate("VotingDialog", "Create Proposal", 0));
        button_SetVotes->setText(QApplication::translate("VotingDialog", "Set Voting Preferences", 0));
    } // retranslateUi

};

namespace Ui {
    class VotingDialog: public Ui_VotingDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VOTINGDIALOG_H
