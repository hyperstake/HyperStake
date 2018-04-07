#include "votingdialog.h"
#include "ui_votingdialog.h"
#include "createproposaldialog.h"
#include "proposalsdialog.h"
#include "setvotesdialog.h"

VotingDialog::VotingDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::VotingDialog)
{
    ui->setupUi(this);
    proposalsDialog = new ProposalsDialog(this);
    createProposalDialog = new CreateProposalDialog(this);
    setVotesDialog = new SetVotesDialog(this);
}

VotingDialog::~VotingDialog()
{
    delete ui;
}

void VotingDialog::SetWalletModel(WalletModel *model)
{
    walletModel = model;
    createProposalDialog->SetWalletModel(model);
    setVotesDialog->SetWalletModel(model);
    proposalsDialog->SetWalletModel(model);
}

void VotingDialog::on_button_CreateProposal_clicked()
{
    createProposalDialog->show();
}

void VotingDialog::on_button_ViewProposals_clicked()
{
    proposalsDialog->show();
    proposalsDialog->UpdateTable();
}

void VotingDialog::on_button_SetVotes_clicked()
{
    setVotesDialog->show();
    setVotesDialog->UpdateTable();
}
