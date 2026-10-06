#pragma once

namespace randomcombat
{
// A widget callback may run above or below our hook in the patcher chain.
// Dispatch only from the dialog procedure, where the native close result can
// be returned to the dialog loop. Never unwind the main menu with an exception.
class MenuLaunchRequest
{
    bool pending = false;

  public:
    void Queue() { pending = true; }
    void Clear() { pending = false; }

    template <typename Message, typename OriginalProc, typename PrepareMessage>
    int Process(Message &message, OriginalProc original, PrepareMessage prepare)
    {
        // A request queued by an outer API hook reaches us on the next message.
        // Handle it before processing any unrelated incoming menu action.
        int result = 0;
        if (!pending)
            result = original(message);
        // If the API hook is below us, its callback has just queued the request.
        if (!pending)
            return result;

        pending = false;
        Message nativeMessage = message;
        prepare(nativeMessage);
        result = original(nativeMessage);
        // The native procedure also replaces the message with the dialog's
        // close command. Preserve that output for the caller's dialog loop.
        message = nativeMessage;
        return result;
    }
};
} // namespace randomcombat
