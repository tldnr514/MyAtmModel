#ifndef ATM_EVENT_H_
#define ATM_EVENT_H_

#include <tinyfsm.hpp>
#include <operation-result.h>

struct Initialized : tinyfsm::Event { };
struct Canceled : tinyfsm::Event { };
struct ErrorOccured : tinyfsm::Event
{
    OperationResult result;
};
struct CardInserted : tinyfsm::Event { };
struct CardEjected : tinyfsm::Event { };
struct CardVerified : tinyfsm::Event { };
struct PinVerified : tinyfsm::Event { };
struct AccountSelected : tinyfsm::Event { };
struct TransactionChosen : tinyfsm::Event { };
struct TransactionContinued : tinyfsm::Event { };
struct TransactionFinished : tinyfsm::Event { };
struct CashRefilled : tinyfsm::Event { };

#endif  // ATM_EVENT_H_