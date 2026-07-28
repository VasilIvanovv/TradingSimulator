import { useState, useRef, useEffect, type FormEvent } from 'react'
import { Outlet, NavLink } from 'react-router-dom'
import { useAuth } from '../context/AuthContext'
import { useAccount } from '../hooks/useAccount'
import { useActiveAccount } from '../context/ActiveAccountContext'
import { ApiError } from '../api/client'

// ── Nav items ─────────────────────────────────────────────────────────────────

const NAV = [
  {
    label: 'Dashboard', to: '/', exact: true,
    icon: (
      <svg viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4" className="w-[13px] h-[13px] shrink-0">
        <rect x="1" y="1" width="5" height="5" rx="0.5"/>
        <rect x="8" y="1" width="5" height="5" rx="0.5"/>
        <rect x="1" y="8" width="5" height="5" rx="0.5"/>
        <rect x="8" y="8" width="5" height="5" rx="0.5"/>
      </svg>
    ),
  },
  {
    label: 'Markets', to: '/markets', exact: false,
    icon: (
      <svg viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4" className="w-[13px] h-[13px] shrink-0">
        <rect x="1" y="8" width="2.5" height="5" rx="0.4"/>
        <rect x="5.75" y="4" width="2.5" height="9" rx="0.4"/>
        <rect x="10.5" y="1" width="2.5" height="12" rx="0.4"/>
      </svg>
    ),
  },
  {
    label: 'Rules', to: '/rules', exact: false,
    icon: (
      <svg viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4" className="w-[13px] h-[13px] shrink-0">
        <line x1="2" y1="4" x2="12" y2="4"/>
        <line x1="2" y1="7" x2="12" y2="7"/>
        <line x1="2" y1="10" x2="8" y2="10"/>
        <circle cx="11" cy="10" r="1.8" fill="currentColor" stroke="none"/>
      </svg>
    ),
  },
  {
    label: 'History', to: '/history', exact: false,
    icon: (
      <svg viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4" className="w-[13px] h-[13px] shrink-0">
        <polyline points="1,10 4,6 7,8 10,3 13,5"/>
      </svg>
    ),
  },
]

// ── Helpers ───────────────────────────────────────────────────────────────────

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

// ── Layout ────────────────────────────────────────────────────────────────────

export function Layout() {
  const { logout } = useAuth()
  const { account } = useAccount()
  const { accounts, activeAccount, setActive, createAccount,
          renameAccount, deleteAccount, deposit, depositAll } = useActiveAccount()

  const [showAccountMenu, setShowAccountMenu] = useState(false)
  const [showDepositModal, setShowDepositModal] = useState(false)
  const [depositAll_, setDepositAll_]           = useState(false)
  const menuRef = useRef<HTMLDivElement>(null)

  const positionCount = account ? Object.keys(account.positions).length : 0

  // Close account menu on outside click
  useEffect(() => {
    function handler(e: MouseEvent) {
      if (menuRef.current && !menuRef.current.contains(e.target as Node))
        setShowAccountMenu(false)
    }
    document.addEventListener('mousedown', handler)
    return () => document.removeEventListener('mousedown', handler)
  }, [])

  return (
    <div className="flex h-screen bg-bg text-txt overflow-hidden">

      {/* ── Sidebar ── */}
      <aside className="w-[196px] shrink-0 bg-surface border-r border-border flex flex-col">

        {/* Brand */}
        <div className="flex items-center gap-[10px] px-[18px] py-[18px] border-b border-border-dim">
          <div className="w-[30px] h-[30px] border border-accent bg-accent/10 rounded flex items-center justify-center font-mono text-[10px] font-bold text-accent shrink-0">
            TS
          </div>
          <div>
            <div className="text-[13px] font-bold text-txt leading-none">TradeSim</div>
            <div className="text-[10px] text-txt-3 uppercase tracking-[0.08em] mt-0.5">Simulator</div>
          </div>
        </div>

        {/* Nav */}
        <nav className="flex-1 py-[10px]">
          <div className="px-[18px] pt-[10px] pb-[3px] text-[10px] uppercase tracking-[0.09em] text-txt-3 font-semibold">
            Overview
          </div>
          {NAV.slice(0, 1).map(item => <NavItem key={item.to} {...item} />)}

          <div className="px-[18px] pt-[14px] pb-[3px] text-[10px] uppercase tracking-[0.09em] text-txt-3 font-semibold">
            Trading
          </div>
          {NAV.slice(1, 3).map(item => <NavItem key={item.to} {...item} />)}

          <div className="px-[18px] pt-[14px] pb-[3px] text-[10px] uppercase tracking-[0.09em] text-txt-3 font-semibold">
            Data
          </div>
          {NAV.slice(3).map(item => <NavItem key={item.to} {...item} />)}
        </nav>

        {/* Account switcher */}
        <div className="border-t border-border-dim px-[12px] py-[10px]" ref={menuRef}>
          <button
            onClick={() => setShowAccountMenu(v => !v)}
            className="w-full flex items-center gap-[8px] px-[8px] py-[7px] rounded hover:bg-white/[0.04] transition-colors text-left"
          >
            <div className="w-[26px] h-[26px] rounded-full bg-accent/20 flex items-center justify-center text-[10px] font-bold text-accent font-mono shrink-0">
              {activeAccount?.name?.[0]?.toUpperCase() ?? '?'}
            </div>
            <div className="flex-1 min-w-0">
              <div className="text-[12px] font-medium text-txt leading-none truncate">
                {activeAccount?.name ?? 'No account'}
              </div>
              <div className="text-[10px] text-txt-3 mt-[2px]">
                {accounts.length} account{accounts.length !== 1 ? 's' : ''}
              </div>
            </div>
            <svg viewBox="0 0 10 10" fill="none" stroke="currentColor" strokeWidth="1.4"
                 className={`w-[9px] h-[9px] text-txt-3 shrink-0 transition-transform ${showAccountMenu ? 'rotate-180' : ''}`}>
              <polyline points="2,3 5,7 8,3"/>
            </svg>
          </button>

          {showAccountMenu && (
            <AccountMenu
              accounts={accounts}
              activeAccount={activeAccount}
              onSelect={a => { setActive(a); setShowAccountMenu(false) }}
              onDeposit={() => { setDepositAll_(false); setShowAccountMenu(false); setShowDepositModal(true) }}
              onDepositAll={() => { setDepositAll_(true); setShowAccountMenu(false); setShowDepositModal(true) }}
              onCreate={createAccount}
              onRename={renameAccount}
              onDelete={deleteAccount}
              onClose={() => setShowAccountMenu(false)}
              onLogout={logout}
            />
          )}
        </div>
      </aside>

      {/* ── Main ── */}
      <div className="flex-1 flex flex-col min-w-0">
        <div className="h-[52px] shrink-0 border-b border-border flex items-center px-[22px]">
          <div className="flex-1" />
          <TopStat label="Cash"      value={account ? `$${fmt(account.cash)}` : '—'} />
          <TopStat label="Positions" value={account ? String(positionCount) : '—'} />
          <TopStat label="Trades"    value={account ? String(account.tradeHistory.length) : '—'} />
        </div>

        <main className="flex-1 overflow-y-auto p-[18px]">
          <Outlet />
        </main>
      </div>

      {/* ── Deposit modal ── */}
      {showDepositModal && (
        <DepositModal
          all={depositAll_}
          activeAccount={activeAccount}
          onDeposit={async (amount) => {
            if (depositAll_) return depositAll(amount)
            if (activeAccount) await deposit(activeAccount.id, amount)
          }}
          onClose={() => setShowDepositModal(false)}
        />
      )}
    </div>
  )
}

// ── Account menu ──────────────────────────────────────────────────────────────

function AccountMenu({
  accounts, activeAccount,
  onSelect, onDeposit, onDepositAll, onCreate, onRename, onDelete, onClose, onLogout,
}: {
  accounts:      ReturnType<typeof useActiveAccount>['accounts']
  activeAccount: ReturnType<typeof useActiveAccount>['activeAccount']
  onSelect:      (a: typeof accounts[number]) => void
  onDeposit:     () => void
  onDepositAll:  () => void
  onCreate:      (name: string) => Promise<unknown>
  onRename:      (id: number, name: string) => Promise<void>
  onDelete:      (id: number) => Promise<void>
  onClose:       () => void
  onLogout:      () => void
}) {
  const [creating,     setCreating]     = useState(false)
  const [newName,      setNewName]      = useState('')
  const [renamingId,   setRenamingId]   = useState<number | null>(null)
  const [renameValue,  setRenameValue]  = useState('')

  async function handleCreate(e: FormEvent) {
    e.preventDefault()
    if (!newName.trim()) return
    await onCreate(newName.trim())
    setNewName('')
    setCreating(false)
  }

  async function handleRename(e: FormEvent, id: number) {
    e.preventDefault()
    if (!renameValue.trim()) return
    await onRename(id, renameValue.trim())
    setRenamingId(null)
  }

  const atCap = accounts.length >= 10

  return (
    <div className="mt-[4px] bg-card border border-border rounded-[6px] overflow-hidden shadow-lg">
      {/* Account list */}
      <div className="max-h-[200px] overflow-y-auto">
        {accounts.map(a => (
          <div key={a.id} className="group flex items-center gap-[6px] px-[10px] py-[7px] hover:bg-surface/60 transition-colors">
            {renamingId === a.id ? (
              <form onSubmit={e => handleRename(e, a.id)} className="flex-1 flex gap-[4px]">
                <input
                  autoFocus
                  value={renameValue}
                  onChange={e => setRenameValue(e.target.value)}
                  className="flex-1 bg-surface border border-accent rounded px-[6px] py-[3px] text-[11px] text-txt outline-none"
                />
                <button type="submit" className="text-[10px] text-accent font-semibold px-[4px]">Save</button>
                <button type="button" onClick={() => setRenamingId(null)} className="text-[10px] text-txt-3 px-[4px]">✕</button>
              </form>
            ) : (
              <>
                <button
                  onClick={() => onSelect(a)}
                  className="flex-1 text-left text-[12px] text-txt truncate"
                >
                  {activeAccount?.id === a.id && (
                    <span className="inline-block w-[6px] h-[6px] rounded-full bg-accent mr-[6px] mb-[1px]" />
                  )}
                  {a.name}
                </button>
                <div className="hidden group-hover:flex items-center gap-[2px]">
                  <button
                    title="Rename"
                    onClick={() => { setRenamingId(a.id); setRenameValue(a.name) }}
                    className="p-[3px] text-txt-3 hover:text-txt transition-colors"
                  >
                    <svg viewBox="0 0 12 12" fill="none" stroke="currentColor" strokeWidth="1.4" className="w-[10px] h-[10px]">
                      <path d="M8 2l2 2-6 6H2V8l6-6z"/>
                    </svg>
                  </button>
                  <button
                    title="Delete"
                    onClick={() => onDelete(a.id)}
                    className="p-[3px] text-txt-3 hover:text-neg transition-colors"
                  >
                    <svg viewBox="0 0 12 12" fill="none" stroke="currentColor" strokeWidth="1.4" className="w-[10px] h-[10px]">
                      <polyline points="2,3 10,3"/><path d="M4,3V2h4v1M5,5v4M7,5v4"/><path d="M3,3l.5,7h5L9,3"/>
                    </svg>
                  </button>
                </div>
              </>
            )}
          </div>
        ))}
      </div>

      <div className="border-t border-border-dim" />

      {/* Create new account */}
      {creating ? (
        <form onSubmit={handleCreate} className="flex items-center gap-[4px] px-[10px] py-[8px]">
          <input
            autoFocus
            value={newName}
            onChange={e => setNewName(e.target.value)}
            placeholder="Account name"
            className="flex-1 bg-surface border border-accent rounded px-[6px] py-[3px] text-[11px] text-txt outline-none placeholder:text-txt-3"
          />
          <button type="submit" className="text-[10px] text-accent font-semibold px-[4px]">Add</button>
          <button type="button" onClick={() => setCreating(false)} className="text-[10px] text-txt-3 px-[4px]">✕</button>
        </form>
      ) : (
        <button
          onClick={() => !atCap && setCreating(true)}
          disabled={atCap}
          className="w-full flex items-center gap-[6px] px-[10px] py-[8px] text-[11px] text-txt-3 hover:text-txt disabled:opacity-40 disabled:cursor-not-allowed transition-colors"
        >
          <span className="text-[14px] leading-none">+</span>
          {atCap ? 'Account limit reached (10)' : 'New account'}
        </button>
      )}

      <div className="border-t border-border-dim" />

      {/* Deposit actions */}
      <button
        onClick={onDeposit}
        className="w-full flex items-center gap-[6px] px-[10px] py-[7px] text-[11px] text-txt-3 hover:text-txt transition-colors"
      >
        <span className="text-[12px]">↓</span> Deposit to this account
      </button>
      <button
        onClick={onDepositAll}
        className="w-full flex items-center gap-[6px] px-[10px] py-[7px] text-[11px] text-txt-3 hover:text-txt transition-colors"
      >
        <span className="text-[12px]">⇊</span> Deposit to all accounts
      </button>

      <div className="border-t border-border-dim" />

      <button
        onClick={() => { onClose(); onLogout() }}
        className="w-full flex items-center gap-[6px] px-[10px] py-[7px] text-[11px] text-txt-3 hover:text-neg transition-colors"
      >
        Sign out
      </button>
    </div>
  )
}

// ── Deposit modal ─────────────────────────────────────────────────────────────

function DepositModal({
  all, activeAccount, onDeposit, onClose,
}: {
  all:           boolean
  activeAccount: ReturnType<typeof useActiveAccount>['activeAccount']
  onDeposit:     (amount: number) => Promise<unknown>
  onClose:       () => void
}) {
  const [amount,      setAmount]      = useState('')
  const [submitting,  setSubmitting]  = useState(false)
  const [error,       setError]       = useState<string | null>(null)
  const [result,      setResult]      = useState<string | null>(null)

  async function handleSubmit(e: FormEvent) {
    e.preventDefault()
    const value = parseFloat(amount)
    if (!value || value <= 0) return
    setError(null)
    setResult(null)
    setSubmitting(true)
    try {
      const res = await onDeposit(value)
      if (res && typeof res === 'object' && 'failed' in res) {
        const r = res as { succeeded: number[]; failed: number[] }
        if (r.failed.length > 0)
          setError(`Deposited to ${r.succeeded.length} account(s). Failed for ${r.failed.length} account(s).`)
        else
          setResult(`$${value.toFixed(2)} deposited to all ${r.succeeded.length} account(s).`)
      } else {
        setResult(`$${value.toFixed(2)} deposited successfully.`)
      }
      setAmount('')
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Deposit failed')
    } finally {
      setSubmitting(false)
    }
  }

  return (
    <div className="fixed inset-0 bg-black/50 flex items-center justify-center z-50" onClick={onClose}>
      <div
        className="bg-card border border-border rounded-[8px] p-[20px] w-[320px] shadow-xl"
        onClick={e => e.stopPropagation()}
      >
        <div className="text-[13px] font-semibold text-txt mb-[4px]">
          {all ? 'Deposit to all accounts' : `Deposit to "${activeAccount?.name}"`}
        </div>
        <div className="text-[11px] text-txt-3 mb-[14px]">
          {all
            ? 'The same amount will be added to every account.'
            : 'Add funds to this account\'s cash balance.'}
        </div>

        <form onSubmit={handleSubmit} className="flex flex-col gap-[10px]">
          <div className="relative">
            <span className="absolute left-3 top-1/2 -translate-y-1/2 text-txt-3 text-[12px] pointer-events-none">$</span>
            <input
              type="number" min="0.01" step="0.01" autoFocus
              value={amount}
              onChange={e => setAmount(e.target.value)}
              onBlur={e => {
                const v = parseFloat(e.target.value)
                if (!isNaN(v)) setAmount((Math.round(v * 100) / 100).toFixed(2))
              }}
              required placeholder="0.00"
              className="w-full bg-surface border border-border rounded pl-6 pr-3 py-[8px] text-txt text-[13px] font-mono outline-none focus:border-accent transition-colors placeholder:text-txt-3"
            />
          </div>

          {error  && <div className="text-neg text-[11px]">{error}</div>}
          {result && <div className="text-pos text-[11px]">{result}</div>}

          <div className="flex gap-[8px] mt-[2px]">
            <button
              type="button" onClick={onClose}
              className="flex-1 py-[8px] rounded border border-border text-[12px] text-txt-2 hover:text-txt transition-colors"
            >
              Cancel
            </button>
            <button
              type="submit" disabled={submitting}
              className="flex-1 py-[8px] rounded bg-accent text-[#050C18] text-[12px] font-semibold hover:brightness-110 disabled:opacity-50 transition-all"
            >
              {submitting ? 'Depositing…' : 'Deposit'}
            </button>
          </div>
        </form>
      </div>
    </div>
  )
}

// ── Sub-components ────────────────────────────────────────────────────────────

function NavItem({ label, to, exact, icon }: typeof NAV[number]) {
  return (
    <NavLink
      to={to}
      end={exact}
      className={({ isActive }) =>
        `flex items-center gap-[9px] px-[18px] py-[8px] text-[12.5px] border-l-2 transition-colors ` +
        (isActive
          ? 'text-accent border-accent bg-accent/10'
          : 'text-txt-2 border-transparent hover:text-txt hover:bg-white/[0.025]')
      }
    >
      {icon}
      {label}
    </NavLink>
  )
}

function TopStat({ label, value }: { label: string; value: string }) {
  return (
    <div className="flex flex-col items-end px-[20px] border-l border-border first:border-l-0">
      <span className="text-[9.5px] uppercase tracking-[0.08em] text-txt-3 font-semibold">{label}</span>
      <span className="text-[13px] font-mono font-semibold tabular-nums text-txt">{value}</span>
    </div>
  )
}
