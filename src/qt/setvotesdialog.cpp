#include "setvotesdialog.h"
#include "ui_setvotesdialog.h"
#include "init.h"
#include "wallet.h"
#include "main.h"
//#include "walletmodel.h"
//#include "voteproposal.h"
#include <QLineEdit>
#include <QMessageBox>
#include <QStandardItemModel>

SetVotesDialog::SetVotesDialog(QWidget* parent) :
        QDialog(parent),
        ui(new Ui::SetVotesDialog)
{
    ui->setupUi(this);
    walletModel = nullptr;
    Clear();
    ui->votesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    UpdateTable();
}

SetVotesDialog::~SetVotesDialog()
{
    delete ui;
}

void SetVotesDialog::SetWalletModel(WalletModel* model)
{
    walletModel = model;
}

void SetVotesDialog::on_voteButton_clicked()
{
    if(pwalletMain->IsLocked()) {
        QMessageBox msg;
        msg.setText(tr("Error: Unlock wallet to vote on a proposal."));
        msg.exec();
        return;
    }

    //Get params
    uint256 txHash(ui->txidLineEdit->text().toStdString());
    int voteChoice = ui->voteComboBox->currentIndex();

    CTransaction tx;
    uint256 hashBlock;
    if(!GetTransaction(txHash, tx, hashBlock)) {
        QMessageBox msg;
        msg.setText(tr("Transaction not found in wallet."));
        msg.exec();
        return;
    }

    if (!tx.IsProposal()) {
        QMessageBox msg;
        msg.setText(tr("Transaction does not contain a proposal."));
        msg.exec();
        return;
    }

    CVoteProposal proposal;
    if (!ProposalFromTransaction(tx, proposal)) {
        QMessageBox msg;
        msg.setText(tr("Proposal couldn't be found in the transaction."));
        msg.exec();
        return;
    }

    CVoteObject voteObject(proposal.GetHash(), proposal.GetBitCount(), proposal.GetShift());

    voteObject.Vote(voteChoice);

    //add the voteObject in the map
    pwalletMain->mapVoteObjects[proposal.GetHash()] = voteObject;

    //write the vote object to the database
    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (!walletdb.WriteVoteObject(proposal.GetHash().GetHex(), voteObject)) {
        QMessageBox msg;
        msg.setText(tr("The vote was saved; however, there were problems writing the vote to the database."));
        msg.exec();
        return;
    }

    QMessageBox msg;
    msg.setText(tr("Vote submitted!\n\nProposal Name: %1\nProposal Description: %2\nYour Vote: "
                           "%3").arg(QString::fromStdString(proposal.GetName())).arg(QString::fromStdString(proposal.GetDescription())).arg(ui->voteComboBox->currentText()));
    msg.exec();
    Clear();
    UpdateTable();
    return;
}

void SetVotesDialog::UpdateTable()
{
    if(pwalletMain == nullptr)
        return;

    int columns = 3;
    int rows = 0;
    QStandardItemModel* model = new QStandardItemModel(rows, columns, this);
    model->setHorizontalHeaderItem(0, new QStandardItem(QString("Name")));
    model->setHorizontalHeaderItem(1, new QStandardItem(QString("Description")));
    model->setHorizontalHeaderItem(2, new QStandardItem(QString("Your Vote")));

    int i = 0;
    CVoteDB votedb("r");
    if(!(pwalletMain->mapVoteObjects.empty())) {
        std::map<uint256, CVoteObject>::iterator it;
        for (it = pwalletMain->mapVoteObjects.begin(); it != pwalletMain->mapVoteObjects.end(); it++) {
            QList < QStandardItem * > listItems;

            uint256 txid = 0;
            for (auto mit : mapProposals) {
                if (mit.second == it->first) {
                    txid = mit.first;
                    break;
                }
            }

            if (txid == 0)
                return;

            CVoteProposal proposal;
            if (!votedb.ReadProposal(txid, proposal)) {
                return;
            }

            int voteValue = it->second.GetVote();
            std::string strVote;

            switch(voteValue) {
                case 1:
                    strVote = "Yes";
                    break;
                case 2:
                    strVote = "No";
                    break;
                case 3:
                    strVote = "Request proposal revision";
                    break;
                default:
                    strVote = "Abstain";
                    break;
            }

            listItems.push_back(new QStandardItem(QString::fromStdString(proposal.GetName())));
            listItems.push_back(new QStandardItem(QString::fromStdString(proposal.GetDescription())));
            listItems.push_back(new QStandardItem(QString::fromStdString(strVote)));

            model->insertRow(i, listItems);
            ++i;
        }
    }

    ui->votesTable->setModel(model);
}

void SetVotesDialog::Clear()
{
    ui->txidLineEdit->clear();
    ui->voteComboBox->setCurrentIndex(0);
}