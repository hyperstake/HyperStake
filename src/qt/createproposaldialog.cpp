#include "createproposaldialog.h"
#include "ui_createproposaldialog.h"
#include "init.h"
#include "main.h"
#include "walletmodel.h"
#include "voteproposal.h"
#include <QLineEdit>
#include <QMessageBox>

CreateProposalDialog::CreateProposalDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::CreateProposalDialog)
{
    ui->setupUi(this);
    walletModel = nullptr;
    Clear();
}

CreateProposalDialog::~CreateProposalDialog()
{
    delete ui;
}

void CreateProposalDialog::SetWalletModel(WalletModel* model)
{
    walletModel = model;
}

void CreateProposalDialog::on_button_CreateProposal_clicked()
{
    QString strName = ui->lineEdit_Name->text();
    if (!strName.size() || strName.size() > MAX_CHAR_NAME) {
        QMessageBox msg;
        msg.setText(tr("Name needs to be between 1 and %1 characters long").arg(MAX_CHAR_NAME));
        msg.exec();
        return;
    }

    QString strAbstract = ui->lineEdit_Abstract->text();
    if (!strAbstract.size() || strAbstract.size() > MAX_CHAR_ABSTRACT) {
        QMessageBox msg;
        msg.setText(tr("Abstract needs to be between 1 and %1 characters long").arg(MAX_CHAR_ABSTRACT));
        msg.exec();
        return;
    }

    int nStartHeight = ui->lineEdit_StartBlock->text().toInt();
    if (nStartHeight < nBestHeight || nStartHeight > nBestHeight + MAX_BLOCKS_IN_FUTURE) {
        QMessageBox msg;
        msg.setText(tr("Start height needs to be greater than current height (%1) and less than %2.").arg(nBestHeight).arg(nStartHeight + MAX_BLOCKS_IN_FUTURE));
        msg.exec();
        return;
    }

    int nCheckSpan = ui->lineEdit_Length->text().toInt();
    if (!nCheckSpan || nCheckSpan > MAX_CHECKSPAN) {
        QMessageBox msg;
        msg.setText(tr("Voting length needs to be between 1 and %1 blocks").arg(MAX_CHECKSPAN));
        msg.exec();
        return;
    }

    //Right now only supporting 2 bit votes
    int nBitCount = 2;
    QString strSize = QString::number(nBitCount);
    ui->label_Size_result->setText(strSize);

    //Set bit location in dialog
    VoteLocation location;
    if (!proposalManager.GetNextLocation(nBitCount, nStartHeight, nCheckSpan, location)) {
        QMessageBox msg;
        msg.setText(tr("Failed to get next location from the proposal manager"));
        msg.exec();
        return;
    }
    ui->label_Location_result->setText(QString::number(location.second));

    //Create the actual proposal
    this->proposal = new CVoteProposal(strName.toStdString(), location.second - 1, nStartHeight, nCheckSpan, nBitCount, strAbstract.toStdString());

    //Set proposal hash in dialog
    uint256 hashProposal = proposal->GetHash();
    QString strHash = QString::fromStdString(hashProposal.GetHex());
    ui->label_Hash_result->setText(strHash);

    if (strHash != "")
        ui->button_SendProposal->setDisabled(false);
}

void CreateProposalDialog::on_button_SendProposal_clicked()
{
    uint256 txid;
    if (!pwalletMain->SendProposal(*proposal, txid)) {
        QMessageBox msg;
        msg.setText(tr("Failed to send proposal"));
        msg.exec();
        return;
    }

    //display dialog showing tx success
    QMessageBox msg;
    msg.setText(tr("Proposal Sent. TXID: %1").arg(QString::fromStdString(txid.GetHex())));
    msg.exec();

    Clear();
}

void CreateProposalDialog::Clear()
{
    ui->lineEdit_Abstract->clear();
    ui->lineEdit_Length->clear();
    ui->lineEdit_Name->clear();
    ui->lineEdit_StartBlock->clear();
    ui->label_Fee_result->setText(QString::fromStdString(FormatMoney(CVoteProposal::FEE)));
    ui->label_Hash_result->setText("(Automatically Generated)");
    ui->label_Location_result->setText("(Automatically Generated)");
    ui->label_Location_result->setText("(Automatically Generated)");
    ui->button_SendProposal->setEnabled(false);
}
