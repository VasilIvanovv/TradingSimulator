import { useState, FormEvent, type ReactNode } from 'react'
import { tradingApi, type ExecutionReceipt } from '../api/trading'
import { ApiError } from '../api/client'
import { SymbolPicker } from '../components/SymbolPicker'

type Side = 'buy' | 'sell'

interface Result {
  receipt:  ExecutionReceipt
  symbol:   string
  side:     Side
  quantity: number
}

export function TradePage() {
  const [symbol,   setSymbol]   = useState('')
  const [side,     setSide]     = useState<Side>('buy')
  const [quantity, setQuantity] = useState('')
  const [price,    setPrice]    = useState('')
  const [loading,  setLoading]  = useState(false)
  const [error,    setError]    = useState<string | null>(null)
  const [result,   setResult]   = useState<Result | null>(null)

  async function handleSubmit(e: FormEvent) {
    e.preventDefault()
    setError(null)
    setResult(null)
    setLoading(true)
    try {
      const receipt = await tradingApi.placeOrder(
        symbol.toUpperCase().trim(),
        side,
        parseFloat(quantity),
        parseFloat(price),
      )
      setResult({ receipt, symbol: symbol.toUpperCase().trim(), side, quantity: parseFloat(quantity) })
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Order failed')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="max-w-sm flex flex-col gap-[14px]">
      <div className="bg-card border border-border rounded-[6px]">
        <div className="px-[16px] py-[12px] border-b border-border-dim">
          <span className="text-[11px] font-semibold uppercase tracking-[0.08em] text-txt-2">Place Order</span>
        </div>

        <form onSubmit={handleSubmit} className="px-[16px] py-[14px] flex flex-col gap-[12px]">

          {/* Side toggle */}
          <div>
            <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold mb-[6px]">Side</div>
            <div className="flex rounded overflow-hidden border border-border">
              <SideButton active={side === 'buy'}  label="Buy"  onClick={() => setSide('buy')}  color="pos" />
              <SideButton active={side === 'sell'} label="Sell" onClick={() => setSide('sell')} color="neg" />
            </div>
          </div>

          {/* Symbol */}
          <Field label="Symbol">
            <SymbolPicker value={symbol} onChange={setSymbol} />
            {/* Hidden required input so the form validates selection */}
            <input type="text" value={symbol} required readOnly className="sr-only" tabIndex={-1} />
          </Field>

          {/* Quantity */}
          <Field label="Quantity">
            <input
              type="number"
              min="0.000001"
              step="any"
              value={quantity}
              onChange={e => setQuantity(e.target.value)}
              required
              placeholder="100"
              className={input + ' font-mono tabular-nums'}
            />
          </Field>

          {/* Limit price */}
          <Field label="Limit Price" hint="per share">
            <div className="relative">
              <span className="absolute left-3 top-1/2 -translate-y-1/2 text-txt-3 text-[12px] pointer-events-none">$</span>
              <input
                type="number"
                min="0.000001"
                step="any"
                value={price}
                onChange={e => setPrice(e.target.value)}
                required
                placeholder="0.00"
                className={input + ' pl-6 font-mono tabular-nums'}
              />
            </div>
          </Field>

          {error && (
            <div className="px-3 py-2 rounded bg-neg/10 border border-neg/30 text-neg text-[11px]">
              {error}
            </div>
          )}

          <button
            type="submit"
            disabled={loading}
            className={
              `mt-1 py-[8px] rounded text-[12px] font-semibold transition-all disabled:opacity-50 disabled:cursor-not-allowed ` +
              (side === 'buy'
                ? 'bg-pos text-[#050C18] hover:brightness-110'
                : 'bg-neg text-white hover:brightness-110')
            }
          >
            {loading ? 'Placing…' : `${side === 'buy' ? 'Buy' : 'Sell'} ${symbol.toUpperCase() || '—'}`}
          </button>
        </form>
      </div>

      {result && <ExecutionCard result={result} onDismiss={() => setResult(null)} />}
    </div>
  )
}

// ── Execution result card ─────────────────────────────────────────────────────

function ExecutionCard({ result, onDismiss }: { result: Result; onDismiss: () => void }) {
  const filled   = result.receipt.status === 'filled'
  const total    = result.quantity * result.receipt.executionPrice

  return (
    <div className={`bg-card border rounded-[6px] px-[16px] py-[14px] flex flex-col gap-[10px] ${filled ? 'border-pos/40' : 'border-neg/40'}`}>
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-[8px]">
          <span className={`text-[11px] font-bold uppercase tracking-wide ${filled ? 'text-pos' : 'text-neg'}`}>
            {filled ? 'Filled' : 'Rejected'}
          </span>
          <span className="text-txt-3 text-[11px]">
            {result.side === 'buy' ? 'Buy' : 'Sell'} {result.quantity} × {result.symbol}
          </span>
        </div>
        <button onClick={onDismiss} className="text-txt-3 hover:text-txt text-[16px] leading-none transition-colors">×</button>
      </div>

      {filled && (
        <div className="grid grid-cols-2 gap-[8px]">
          <Stat label="Execution Price" value={`$${fmt(result.receipt.executionPrice)}`} />
          <Stat label="Total Value"     value={`$${fmt(total)}`} />
        </div>
      )}
    </div>
  )
}

// ── Primitives ────────────────────────────────────────────────────────────────

function SideButton({ active, label, onClick, color }: { active: boolean; label: string; onClick: () => void; color: 'pos' | 'neg' }) {
  const activeClass = color === 'pos'
    ? 'bg-pos/20 text-pos'
    : 'bg-neg/20 text-neg'
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

function Stat({ label, value }: { label: string; value: string }) {
  return (
    <div className="bg-surface rounded px-[10px] py-[8px]">
      <div className="text-[9.5px] uppercase tracking-[0.07em] text-txt-3 font-semibold mb-[3px]">{label}</div>
      <div className="text-[13px] font-mono tabular-nums text-txt font-semibold">{value}</div>
    </div>
  )
}

const input =
  'bg-surface border border-border rounded px-3 py-[7px] text-txt text-[12px] outline-none ' +
  'focus:border-accent transition-colors placeholder:text-txt-3 w-full'

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}
