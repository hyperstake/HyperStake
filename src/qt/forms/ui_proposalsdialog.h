/********************************************************************************
** Form generated from reading UI file 'proposalsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROPOSALSDIALOG_H
#define UI_PROPOSALSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ProposalsDialog
{
public:
    QDialogButtonBox *buttonBox;
    QWidget *horizontalLayoutWidget;
    QHBoxLayout *horizontalLayout;
    QTableView *tableView;
    QVBoxLayout *verticalLayout;
    QCheckBox *checkBox_Active;
    QCheckBox *checkBox_Upcoming;
    QCheckBox *checkBox_Finished;
    QSpacerItem *horizontalSpacer;
    QSpacerItem *verticalSpacer;

    void setupUi(QDialog *ProposalsDialog)
    {
        if (ProposalsDialog->objectName().isEmpty())
            ProposalsDialog->setObjectName(QStringLiteral("ProposalsDialog"));
        ProposalsDialog->resize(767, 443);
        buttonBox = new QDialogButtonBox(ProposalsDialog);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setGeometry(QRect(380, 400, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        horizontalLayoutWidget = new QWidget(ProposalsDialog);
        horizontalLayoutWidget->setObjectName(QStringLiteral("horizontalLayoutWidget"));
        horizontalLayoutWidget->setGeometry(QRect(20, 10, 741, 381));
        horizontalLayout = new QHBoxLayout(horizontalLayoutWidget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        tableView = new QTableView(horizontalLayoutWidget);
        tableView->setObjectName(QStringLiteral("tableView"));

        horizontalLayout->addWidget(tableView);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(-1, -1, 0, -1);
        checkBox_Active = new QCheckBox(horizontalLayoutWidget);
        checkBox_Active->setObjectName(QStringLiteral("checkBox_Active"));
        checkBox_Active->setChecked(true);

        verticalLayout->addWidget(checkBox_Active);

        checkBox_Upcoming = new QCheckBox(horizontalLayoutWidget);
        checkBox_Upcoming->setObjectName(QStringLiteral("checkBox_Upcoming"));
        checkBox_Upcoming->setChecked(true);

        verticalLayout->addWidget(checkBox_Upcoming);

        checkBox_Finished = new QCheckBox(horizontalLayoutWidget);
        checkBox_Finished->setObjectName(QStringLiteral("checkBox_Finished"));
        checkBox_Finished->setChecked(true);

        verticalLayout->addWidget(checkBox_Finished);

        horizontalSpacer = new QSpacerItem(100, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        verticalLayout->addItem(horizontalSpacer);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        horizontalLayout->addLayout(verticalLayout);


        retranslateUi(ProposalsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), ProposalsDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), ProposalsDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(ProposalsDialog);
    } // setupUi

    void retranslateUi(QDialog *ProposalsDialog)
    {
        ProposalsDialog->setWindowTitle(QApplication::translate("ProposalsDialog", "Vote Proposals", 0));
        checkBox_Active->setText(QApplication::translate("ProposalsDialog", "Active", 0));
        checkBox_Upcoming->setText(QApplication::translate("ProposalsDialog", "Upcoming", 0));
        checkBox_Finished->setText(QApplication::translate("ProposalsDialog", "Finished", 0));
    } // retranslateUi

};

namespace Ui {
    class ProposalsDialog: public Ui_ProposalsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROPOSALSDIALOG_H
