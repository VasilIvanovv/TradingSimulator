import { useState, useEffect, type FormEvent, type ReactNode } from 'react'
import { tradingApi, type ActiveRule } from '../api/trading'
import { ApiError, BASE } from '../api/client'
import { SymbolPicker } from '../components/SymbolPicker'

type Side = 'buy' | 'sell'

export function RulesPage() {
  const [rules,      setRules]      = useState<ActiveRule[]>([])
  const [listLoading, setListLoading] = useState(true)
  const [listError,  setListError]  = useState<string | null>(null)

  const [symbol,       setSymbol]       = useState('')
  const [side,         setSide]         = useState<Side>('buy')
  const [triggerPrice, setTriggerPrice] = useState('')
  const [quantity,     setQuantity]     = useState('')
  const [submitting,   setSubmitting]   = useState(false)
  const [formError,    setFormError]    = useState<string | null>(null)

  async function loadRules() {
    setListError(null)
    try {
      setRules(await tradingApi.getRules())
    } catch (err) {
      setListError(err instanceof ApiError ? err.message : 'Failed to load rules')
    } finally {
      setListLoading(false)
    }
  }

  useEffect(() => { loadRules() }, [])

  async function handleDelete(sym: string) {
    try {
      await tradingApi.removeRules(sym)
      await loadRules()
    } catch (err) {
      setListError(err instanceof ApiError ? err.message : 'Failed to remove rule')
    }
  }

  async function handleSubmit(e: FormEvent) {
    e.preventDefault()
    setFormError(null)
    setSubmitting(true)
    try {
      await tradingApi.addRule(
        symbol.toUpperCase().trim(),
        parseFloat(triggerPrice),
        side,
        parseFloat(quantity),
      )
      setSymbol('')
      setTriggerPrice('')
      setQuantity('')
      setSide('buy')
      await loadRules()
    } catch (err) {
      setFormError(err instanceof ApiError ? err.message : 'Failed to add rule')
    } finally {
      setSubmitting(false)
    }
  }

  return (
    <div className="max-w-sm flex flex-col gap-[14px]">

      {/* ── Active rules list ─────────────────────────────────────────────── */}
      <div className="bg-card border border-border rounded-[6px]">
        <div className="px-[16px] py-[12px] border-b border-border-dim flex items-center justify-between">
          <span className="text-[11px] font-semibold uppercase tracking-[0.08em] text-txt-2">Active Rules</span>
          {!listLoading && (
            <span className="text-[10px] text-txt-3">{rules.length} active</span>
          )}
        </div>

        {listError && (
          <div className="px-[16px] py-[10px] border-b border-border-dim flex items-center justify-between">
            <span className="text-neg text-[11px]">{listError}</span>
            <button
              onClick={() => { setListError(null); setListLoading(true); loadRules() }}
              className="text-link text-[10px]"
            >
              Retry
            </button>
          </div>
        )}

        {listLoading ? (
          <div className="divide-y divide-border-dim">
            {[1, 2].map(i => (
              <div key={i} className="px-[16px] py-[12px] flex items-center gap-[10px]">
                <div className="w-[32px] h-[32px] rounded-full bg-surface animate-pulse" />
                <div className="flex-1 flex flex-col gap-[5px]">
                  <div className="h-[11px] w-20 bg-surface rounded animate-pulse" />
                  <div className="h-[9px] w-32 bg-surface rounded animate-pulse" />
                </div>
              </div>
            ))}
          </div>
        ) : rules.length === 0 && !listError ? (
          <div className="px-[16px] py-[24px] text-center text-txt-3 text-[12px]">
            No active rules. Add one below.
          </div>
        ) : (
          <div className="divide-y divide-border-dim">
            {rules.map((rule, i) => (
              <RuleRow key={`${rule.symbol}-${i}`} rule={rule} onDelete={handleDelete} />
            ))}
          </div>
        )}
      </div>

      {/* ── Add rule form ─────────────────────────────────────────────────── */}
      <div className="bg-card border border-border rounded-[6px]">
        <div className="px-[16px] py-[12px] border-b border-border-dim">
          <span className="text-[11px] font-semibold uppercase tracking-[0.08em] text-txt-2">Add Rule</span>
        </div>

        <form onSubmit={handleSubmit} className="px-[16px] py-[14px] flex flex-col gap-[12px]">
          <div>
            <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold mb-[6px]">Trigger Side</div>
            <div className="flex rounded overflow-hidden border border-border">
              <SideButton active={side === 'buy'}  label="Buy"  onClick={() => setSide('buy')}  color="pos" />
              <SideButton active={side === 'sell'} label="Sell" onClick={() => setSide('sell')} color="neg" />
            </div>
          </div>

          <Field label="Symbol">
            <SymbolPicker value={symbol} onChange={setSymbol} />
            <input type="text" value={symbol} required readOnly className="sr-only" tabIndex={-1} />
          </Field>

          <Field label="Trigger Price" hint="per share">
            <div className="relative">
              <span className="absolute left-3 top-1/2 -translate-y-1/2 text-txt-3 text-[12px] pointer-events-none">$</span>
              <input
                type="number"
                min="0.000001"
                step="any"
                value={triggerPrice}
                onChange={e => setTriggerPrice(e.target.value)}
                required
                placeholder="0.00"
                className={inputCls + ' pl-6 font-mono tabular-nums'}
              />
            </div>
          </Field>

          <Field label="Quantity">
            <input
              type="number"
              min="0.000001"
              step="any"
              value={quantity}
              onChange={e => setQuantity(e.target.value)}
              required
              placeholder="100"
              className={inputCls + ' font-mono tabular-nums'}
            />
          </Field>

          {formError && (
            <div className="px-3 py-2 rounded bg-neg/10 border border-neg/30 text-neg text-[11px]">
              {formError}
            </div>
          )}

          <button
            type="submit"
            disabled={submitting}
            className="mt-1 py-[8px] rounded text-[12px] font-semibold bg-accent text-[#050C18]
                       hover:brightness-110 transition-all disabled:opacity-50 disabled:cursor-not-allowed"
          >
            {submitting ? 'Adding…' : 'Add Rule'}
          </button>
        </form>
      </div>
    </div>
  )
}

// ── Rule row ──────────────────────────────────────────────────────────────────

function RuleRow({ rule, onDelete }: { rule: ActiveRule; onDelete: (s: string) => Promise<void> }) {
  const [deleting, setDeleting] = useState(false)

  async function handleDelete() {
    setDeleting(true)
    await onDelete(rule.symbol)
    setDeleting(false)
  }

  return (
    <div className="px-[16px] py-[11px] flex items-center gap-[10px]">
      <SymbolLogo symbol={rule.symbol} size={32} />

      <div className="flex-1 min-w-0">
        <div className="flex items-center gap-[6px] mb-[2px]">
          <span className="text-[12.5px] font-medium text-txt font-mono">{rule.symbol}</span>
          <span className={
            `text-[9px] font-bold uppercase tracking-wide px-[5px] py-[1px] rounded ` +
            (rule.side === 'buy' ? 'bg-pos/15 text-pos' : 'bg-neg/15 text-neg')
          }>
            {rule.side}
          </span>
        </div>
        <div className="text-[10.5px] text-txt-3">
          Trigger at{' '}
          <span className="font-mono text-txt-2">
            ${rule.triggerPrice.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}
          </span>
          {' · '}
          <span className="font-mono">{rule.quantity}</span> shares
        </div>
      </div>

      <button
        onClick={handleDelete}
        disabled={deleting}
        title="Remove rule"
        className="text-txt-3 hover:text-neg transition-colors text-[18px] leading-none disabled:opacity-40 ml-1"
      >
        {deleting ? '…' : '×'}
      </button>
    </div>
  )
}

// ── Symbol logo ───────────────────────────────────────────────────────────────

const AVATAR_COLORS = [
  '#1A3A5C', '#1A4A3A', '#3A1A4A', '#4A2A1A',
  '#1A3A4A', '#3A3A1A', '#1A1A4A', '#3A1A1A',
]

function SymbolLogo({ symbol, size }: { symbol: string; size: number }) {
  const [failed, setFailed] = useState(false)
  const style = { width: size, height: size, minWidth: size }

  if (failed) {
    const bg = AVATAR_COLORS[symbol.charCodeAt(0) % AVATAR_COLORS.length]
    return (
      <div
        style={{ ...style, background: bg, fontSize: Math.round(size * 0.38) }}
        className="rounded-full flex items-center justify-center font-mono font-bold text-txt-2"
      >
        {symbol[0]}
      </div>
    )
  }
  return (
    <img
      src={`${BASE}/logos/${symbol}`}
      alt={symbol}
      style={style}
      loading="lazy"
      className="rounded-full object-contain"
      onError={() => setFailed(true)}
    />
  )
}

// ── Primitives ────────────────────────────────────────────────────────────────

function SideButton({ active, label, onClick, color }: {
  active: boolean; label: string; onClick: () => void; color: 'pos' | 'neg'
}) {
  const activeClass = color === 'pos' ? 'bg-pos/20 text-pos' : 'bg-neg/20 text-neg'
  return (
    <button
      type="button"
      onClick={onClick}
      className={
        `flex-1 py-[7px] text-[12px] font-semibold transition-colors ` +
        (active ? activeClass : 'text-txt-3 hover:text-txt bg-transparent')
      }
    >
      {label}
    </button>
  )
}

function Field({ label, hint, children }: { label: string; hint?: string; children: ReactNode }) {
  return (
    <label className="flex flex-col gap-[5px]">
      <div className="flex items-center gap-2">
        <span className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold">{label}</span>
        {hint && <span className="text-[10px] text-txt-3">{hint}</span>}
      </div>
      {children}
    </label>
  )
}

const inputCls =
  'bg-surface border border-border rounded px-3 py-[7px] text-txt text-[12px] outline-none ' +
  'focus:border-accent transition-colors placeholder:text-txt-3 w-full'
