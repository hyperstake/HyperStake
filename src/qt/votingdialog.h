#ifndef VOTINGDIALOG_H
#define VOTINGDIALOG_H

#include <QDialog>
#include <QObject>

class CreateProposalDialog;
class ProposalsDialog;
class SetVotesDialog;
class WalletModel;

namespace Ui {
class VotingDialog;
}

class VotingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VotingDialog(QWidget* parent = 0);
    ~VotingDialog();
    void SetWalletModel(WalletModel* model);

public slots:
    void on_button_ViewProposals_clicked();
    void on_button_CreateProposal_clicked();
    void on_button_SetVotes_clicked();

private:
    Ui::VotingDialog *ui;
    CreateProposalDialog* createProposalDialog;
    ProposalsDialog* proposalsDialog;
    SetVotesDialog* setVotesDialog;
    WalletModel* walletModel;
};

#endif // VOTINGDIALOG_H
