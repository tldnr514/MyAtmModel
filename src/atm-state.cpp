#include "atm-state.h"
#include "atm-machine.h"

FSM_INITIAL_STATE(AtmState, InitializingState)

std::function<void(AtmStateEnum)> AtmState::stateCallback_ = [](AtmStateEnum){};
std::shared_ptr<BankServer> AtmState::bankServer_ = nullptr;
std::shared_ptr<CashBin> AtmState::cashBin_ = nullptr;
std::shared_ptr<CardReader> AtmState::cardReader_ = nullptr;
CashCard AtmState::cashCard_ = CashCard();
PinNumber AtmState::pinNumber_ = PinNumber();
AccountSession AtmState::accountSession_ = AccountSession();

AtmStateEnum InitializingState::getState() const
{
    return AtmStateEnum::InitializingState;
}

void InitializingState::react(const Initialized &event)
{
    transit<IdleState>();
}

OperationResult InitializingState::initialize(
        std::shared_ptr<BankServer> bankServer,
        std::shared_ptr<CashBin> cashBin,
        std::shared_ptr<CardReader> cardReader)
{
    bankServer_ = bankServer;
    cashBin_ = cashBin;
    cardReader_ = cardReader;
    AtmState::dispatch(Initialized());
    return OperationResult(ErrorCode::Ok, "");
}

AtmStateEnum IdleState::getState() const
{
    return AtmStateEnum::IdleState;
}

void IdleState::react(const ErrorOccured &event)
{
    if (event.result.isFatal())
        transit<OutOfOrderState>();
}

void IdleState::react(const CardInserted &event)
{
    transit<ReadingCardState>();
}

AtmStateEnum ReadingCardState::getState() const
{
    return AtmStateEnum::ReadingCardState;
}

void ReadingCardState::react(const ErrorOccured &event)
{
    if (event.result.isFatal()) {
        transit<OutOfOrderState>();
    } else {
        transit<EjectingCardState>();
    }
}

void ReadingCardState::react(const CardVerified &event)
{
    transit<ReadingPinState>();
}

void ReadingCardState::entry()
{
    AtmState::entry();
    
    CashCard cashCard;
    const auto readCardResult = cardReader_->readCard(cashCard);
    if (!readCardResult.isSucceed()) {
        AtmState::dispatch(ErrorOccured(readCardResult));
        return;
    }

    const auto verifyCardResult = bankServer_->verifyCard(cashCard);
    if (!verifyCardResult.isSucceed()) {
        AtmState::dispatch(ErrorOccured(verifyCardResult));
        return;
    }

    cashCard_ = cashCard;
    AtmState::dispatch(CardVerified());
}

AtmStateEnum ReadingPinState::getState() const
{
    return AtmStateEnum::ReadingPinState;
}

void ReadingPinState::react(const Canceled &event)
{
    transit<EjectingCardState>();
}

void ReadingPinState::react(const ErrorOccured &event)
{
    if (event.result.isFatal()) {
        transit<OutOfOrderState>();
    } else {
        transit<EjectingCardState>();
    }
}

void ReadingPinState::react(const PinVerified &event)
{
    transit<SelectingAccountState>();
}

OperationResult ReadingPinState::enterPin(const PinNumber &pinNumber)
{
    const auto result = bankServer_->verifyPinNumber(cashCard_, pinNumber);
    
    if (result.isSucceed()) {
        pinNumber_ = pinNumber;
        AtmState::dispatch(PinVerified());
    } else {
        AtmState::dispatch(ErrorOccured(result));
    }
    
    return result;
}

AtmStateEnum SelectingAccountState::getState() const
{
    return AtmStateEnum::SelectingAccountState;
}

void SelectingAccountState::react(const Canceled &event)
{
    transit<EjectingCardState>();
}

void SelectingAccountState::react(const ErrorOccured &event)
{
    if (event.result.isFatal()) {
        transit<OutOfOrderState>();
    } else {
        transit<EjectingCardState>();
    }
}

void SelectingAccountState::react(const AccountSelected &event)
{
    transit<ChoosingTransactionState>();
}

OperationResult SelectingAccountState::selectAccount(AccountType accountType)
{
    AccountSession accountSession;
    const auto result = bankServer_->selectAccount(cashCard_, pinNumber_, accountType, accountSession);

    if (result.isSucceed()) {
        accountSession_ = accountSession;
        AtmState::dispatch(AccountSelected());
    } else {
        AtmState::dispatch(ErrorOccured(result));
    }

    return result;
}

AtmStateEnum ChoosingTransactionState::getState() const
{
    return AtmStateEnum::ChoosingTransactionState;
}

void ChoosingTransactionState::react(const Canceled &event)
{
    transit<EjectingCardState>();
}

void ChoosingTransactionState::react(const ErrorOccured &event)
{
    if (event.result.isFatal()) {
        transit<OutOfOrderState>();
    } else {
        transit<EjectingCardState>();
    }
}

void ChoosingTransactionState::react(const TransactionChosen &event)
{
    transit<PerformingTransactionState>();
}

AtmStateEnum PerformingTransactionState::getState() const
{
    return AtmStateEnum::PerformingTransactionState;
}

void PerformingTransactionState::react(const Canceled &event)
{
    transit<EjectingCardState>();
}

void PerformingTransactionState::react(const ErrorOccured &event)
{
    if (event.result.isFatal()) {
        transit<OutOfOrderState>();
    } else if (event.result.getCode() == ErrorCode::OutOfCash) {
        transit<OutOfCashState>();
    } else {
        transit<EjectingCardState>();
    }
}

void PerformingTransactionState::react(const TransactionContinued &event)
{
    transit<ChoosingTransactionState>();
}

void PerformingTransactionState::react(const TransactionFinished &event)
{
    transit<EjectingCardState>();
}

AtmStateEnum EjectingCardState::getState() const
{
    return AtmStateEnum::EjectingCardState;
}

void EjectingCardState::react(const CardPulledOut &event)
{
    transit<IdleState>();
}

void EjectingCardState::entry()
{
    AtmState::entry();

    cashCard_ = CashCard();
    pinNumber_ = PinNumber();
    accountSession_ = AccountSession();

    if (cardReader_->hasCard())
        cardReader_->ejectCard();
}

AtmStateEnum OutOfCashState::getState() const
{
    return AtmStateEnum::OutOfCashState;
}

void OutOfCashState::react(const CashRefilled &event)
{
    transit<IdleState>();
}

void OutOfCashState::entry()
{
    AtmState::entry();

    cashCard_ = CashCard();
    pinNumber_ = PinNumber();
    accountSession_ = AccountSession();

    if (cardReader_->hasCard())
        cardReader_->ejectCard();
}

AtmStateEnum OutOfOrderState::getState() const
{
    return AtmStateEnum::OutOfOrderState;
}

void OutOfOrderState::entry()
{
    AtmState::entry();

    cashCard_ = CashCard();
    pinNumber_ = PinNumber();
    accountSession_ = AccountSession();

    if (cardReader_->hasCard())
        cardReader_->ejectCard();
}
