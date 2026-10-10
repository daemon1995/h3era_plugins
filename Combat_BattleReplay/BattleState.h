#pragma once

namespace battleReplay
{
struct ReplayContext
{
    bool allowed = false;
    bool requested = false;
};

// ERA FastQuit unwinds native/SEH frames. Lifecycle invalidation must not
// depend on a C++ destructor running or dereference an abandoned stack frame.
class ReplayRuntime
{
    ReplayContext *current_ = nullptr;
    unsigned int generation_ = 0;

  public:
    ReplayContext *Current() const { return current_; }
    unsigned int Generation() const { return generation_; }
    void Reset()
    {
        current_ = nullptr;
        ++generation_;
    }

    class Scope
    {
        ReplayRuntime &runtime_;
        ReplayContext *context_;
        ReplayContext *previous_;
        unsigned int generation_;

      public:
        Scope(ReplayRuntime &runtime, ReplayContext &context)
            : runtime_(runtime), context_(&context), previous_(runtime.current_),
              generation_(runtime.generation_)
        {
            runtime_.current_ = context_;
        }
        bool IsCurrent() const
        {
            return runtime_.generation_ == generation_ && runtime_.current_ == context_;
        }
        ~Scope()
        {
            if (IsCurrent())
                runtime_.current_ = previous_;
        }
        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;
    };
};

// One replayable root battle owns temporary quick-combat settings. Keep the
// backup outside its stack so lifecycle handlers can restore it on FastQuit.
template <class HdVariable> class QuickBattleBackup
{
    bool active_ = false;
    int gameValue_ = 0;
    HdVariable *hdVariable_ = nullptr;
    unsigned int hdValue_ = 0;

  public:
    void Capture(int gameValue, HdVariable *hdVariable)
    {
        gameValue_ = gameValue;
        hdVariable_ = hdVariable;
        hdValue_ = hdVariable ? hdVariable->GetValue() : 0;
        active_ = true;
    }
    void Disable(int &gameValue) const
    {
        if (!active_)
            return;
        gameValue = 0;
        if (hdVariable_)
            hdVariable_->SetValue(0);
    }
    void Restore(int &gameValue)
    {
        if (!active_)
            return;
        active_ = false;
        gameValue = gameValue_;
        if (hdVariable_)
            hdVariable_->SetValue(hdValue_);
        hdVariable_ = nullptr;
    }

    class Scope
    {
        ReplayRuntime &runtime_;
        QuickBattleBackup &backup_;
        int &gameValue_;
        unsigned int generation_;

      public:
        Scope(ReplayRuntime &runtime, QuickBattleBackup &backup, int &gameValue, HdVariable *hdVariable)
            : runtime_(runtime), backup_(backup), gameValue_(gameValue), generation_(runtime.Generation())
        {
            backup_.Capture(gameValue_, hdVariable);
        }
        void Disable() const
        {
            if (generation_ == runtime_.Generation())
                backup_.Disable(gameValue_);
        }
        ~Scope()
        {
            if (generation_ == runtime_.Generation())
                backup_.Restore(gameValue_);
        }
        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;
    };
};

inline bool IsPlayer(int owner)
{
    return owner >= 0 && owner < 8;
}

inline bool CanReplay(bool networkGame, bool humanAttacker, bool humanDefender)
{
    return (humanAttacker || humanDefender) && !(networkGame && humanAttacker && humanDefender);
}

template <class T> inline void CopyObject(T &target, const T &source)
{
    target = source;
}

// Capture the actual argument object, even when an associated hero also exists.
// Non-POD game types can specialize CopyObject to preserve member ownership.
template <class T> class Snapshot
{
    T *target_;
    T saved_{};

  public:
    explicit Snapshot(T *target) : target_(target)
    {
        if (target_)
            CopyObject(saved_, *target_);
    }

    void Restore() const
    {
        if (target_)
            CopyObject(*target_, saved_);
    }

    Snapshot(const Snapshot &) = delete;
    Snapshot &operator=(const Snapshot &) = delete;
};

template <class Hero, class Army> class SideSnapshot
{
    Snapshot<Hero> hero_;
    Snapshot<Army> army_;

  public:
    SideSnapshot(Hero *hero, Army *army) : hero_(hero), army_(army)
    {
    }

    void Restore() const
    {
        hero_.Restore();
        // Must follow the hero restore: valid both for a separate network army
        // and for an army embedded in the hero.
        army_.Restore();
    }
};
} // namespace battleReplay
