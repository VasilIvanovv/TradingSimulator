import { useState, useMemo } from 'react'
import { Link } from 'react-router-dom'
import { useAccount } from '../hooks/useAccount'
import { type TradeRecord } from '../api/trading'

// ── Helpers ───────────────────────────────────────────────────────────────────

function formatTimestamp(ts: string): string {
  if (!ts) return '—'
  const d = new Date(ts.replace(' ', 'T'))
  if (isNaN(d.getTime())) return ts
  return d.toLocaleString('en-US', {
    month: 'short', day: 'numeric', year: 'numeric',
    hour: '2-digit', minute: '2-digit',
  })
}

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}

function fmtQty(n: number) {
  const s = n.toFixed(6).replace(/\.?0+$/, '')
  return s.includes('.') ? s : s
}

type SideFilter = 'all' | 'buy' | 'sell'

// ── Page ──────────────────────────────────────────────────────────────────────

export function HistoryPage() {
  const { account, loading, error, refresh } = useAccount()
  const [search,     setSearch]     = useState('')
  const [sideFilter, setSideFilter] = useState<SideFilter>('all')

  const trades: TradeRecord[] = useMemo(() => {
    if (!account) return []
    return [...account.tradeHistory].reverse()
  }, [account])

  const filtered = useMemo(() => {
    const q = search.trim().toLowerCase()
    return trades.filter(t => {
      if (sideFilter !== 'all' && t.side !== sideFilter) return false
      if (q && !t.symbol.toLowerCase().includes(q)) return false
      return true
    })
  }, [trades, search, sideFilter])

  const stats = useMemo(() => {
    const bought = trades.filter(t => t.side === 'buy')
    const sold   = trades.filter(t => t.side === 'sell')
    const volBought = bought.reduce((s, t) => s + t.quantity * t.executionPrice, 0)
    const volSold   = sold.reduce((s, t) => s + t.quantity * t.executionPrice, 0)
    return {
      total:      trades.length,
      buyCount:   bought.length,
      sellCount:  sold.length,
      netInvested: volBought - volSold,
    }
  }, [trades])

  if (loading) return <Skeleton />
  if (error)   return <ErrorBanner message={error} onRetry={refresh} />

  return (
    <div className="flex flex-col gap-[14px]">
      {/* Stats */}
      <div className="grid grid-cols-4 gap-[14px]">
        <StatCard label="Total Trades"  value={String(stats.total)} />
        <StatCard label="Buys"          value={String(stats.buyCount)}   accent="pos" />
        <StatCard label="Sells"         value={String(stats.sellCount)}  accent="neg" />
        <StatCard label="Net Invested"  value={`$${fmt(stats.netInvested)}`} />
      </div>

      {/* Trade table */}
      <div className="bg-card border border-border rounded-[6px] flex flex-col">
        {/* Card header */}
        <div className="flex items-center justify-between px-[16px] py-[12px] border-b border-border-dim gap-3">
          <div className="flex items-baseline gap-[8px]">
            <span className="text-[11px] font-semibold uppercase tracking-[0.08em] text-txt-2">
              Trade History
            </span>
            <span className="text-[10px] font-mono text-txt-3">{filtered.length}</span>
          </div>

          <div className="flex items-center gap-[8px]">
            {/* Symbol search */}
            <input
              type="text"
              placeholder="Search symbol…"
              value={search}
              onChange={e => setSearch(e.target.value)}
              className="
                text-[11px] font-mono bg-transparent border border-border-dim rounded-[4px]
                px-[8px] py-[4px] text-txt placeholder:text-txt-3
                focus:outline-none focus:border-border w-[140px]
              "
            />

            {/* Side filter */}
            <div className="flex rounded-[4px] border border-border-dim overflow-hidden">
              {(['all', 'buy', 'sell'] as SideFilter[]).map(s => (
                <button
                  key={s}
                  onClick={() => setSideFilter(s)}
                  className={`
                    text-[9px] font-semibold uppercase tracking-wide px-[8px] py-[4px] transition-colors
                    ${s === sideFilter
                      ? 'bg-white/10 text-txt'
                      : 'text-txt-3 hover:text-txt-2'
                    }
                  `}
                >
                  {s}
                </button>
              ))}
            </div>
          </div>
        </div>

        {/* Table */}
        {filtered.length === 0 ? (
          <div className="flex items-center justify-center py-[48px] text-[12px] text-txt-3">
            {trades.length === 0 ? 'No trades yet' : 'No trades match the current filter'}
          </div>
        ) : (
          <div className="overflow-x-auto">
            <table className="w-full text-[12px]">
              <thead>
                <tr className="text-txt-3 text-[10px] uppercase tracking-[0.07em] border-b border-border-dim">
                  <th className="text-left  px-[16px] py-[8px] font-semibold">Date</th>
                  <th className="text-left  px-[4px]  py-[8px] font-semibold">Symbol</th>
                  <th className="text-left  px-[4px]  py-[8px] font-semibold">Side</th>
                  <th className="text-right px-[4px]  py-[8px] font-semibold">Quantity</th>
                  <th className="text-right px-[4px]  py-[8px] font-semibold">Order Price</th>
                  <th className="text-right px-[4px]  py-[8px] font-semibold">Exec Price</th>
                  <th className="text-right px-[4px]  py-[8px] font-semibold">Total</th>
                  <th className="text-right px-[16px] py-[8px] font-semibold">Status</th>
                </tr>
              </thead>
              <tbody>
                {filtered.map((t, i) => {
                  const total = t.quantity * t.executionPrice
                  return (
                    <tr key={i} className="border-b border-border-dim last:border-0 hover:bg-white/[0.02] transition-colors">
                      <td className="px-[16px] py-[10px] text-txt-3 whitespace-nowrap">
                        {formatTimestamp(t.timestamp)}
                      </td>
                      <td className="px-[4px] py-[10px]">
                        <Link
                          to={`/stocks/${t.symbol}`}
                          className="font-mono text-txt hover:text-pos transition-colors"
                        >
                          {t.symbol}
                        </Link>
                      </td>
                      <td className="px-[4px] py-[10px]">
                        <span className={`
                          text-[10px] font-semibold uppercase tracking-wide
                          ${t.side === 'buy' ? 'text-pos' : 'text-neg'}
                        `}>
                          {t.side}
                        </span>
                      </td>
                      <td className="px-[4px] py-[10px] text-right font-mono tabular-nums text-txt-2">
                        {fmtQty(t.quantity)}
                      </td>
                      <td className="px-[4px] py-[10px] text-right font-mono tabular-nums text-txt-2">
                        ${fmt(t.price)}
                      </td>
                      <td className="px-[4px] py-[10px] text-right font-mono tabular-nums text-txt-2">
                        ${fmt(t.executionPrice)}
                      </td>
                      <td className="px-[4px] py-[10px] text-right font-mono tabular-nums text-txt font-medium">
                        ${fmt(total)}
                      </td>
                      <td className="px-[16px] py-[10px] text-right">
                        <span className={`
                          text-[10px] font-semibold uppercase tracking-wide
                          ${t.status === 'filled' ? 'text-pos' : 'text-neg'}
                        `}>
                          {t.status}
                        </span>
                      </td>
                    </tr>
                  )
                })}
              </tbody>
            </table>
          </div>
        )}
      </div>
    </div>
  )
}

// ── Primitives ────────────────────────────────────────────────────────────────

function StatCard({
  label, value, accent,
}: {
  label: string
  value: string
  accent?: 'pos' | 'neg'
}) {
  return (
    <div className="bg-card border border-border rounded-[6px] px-[16px] py-[14px]">
      <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold mb-[6px]">
        {label}
      </div>
      <div className={`text-[22px] font-mono font-semibold tabular-nums ${
        accent === 'pos' ? 'text-pos' : accent === 'neg' ? 'text-neg' : 'text-txt'
      }`}>
        {value}
      </div>
    </div>
  )
}

function Skeleton() {
  return (
    <div className="flex flex-col gap-[14px] animate-pulse">
      <div className="grid grid-cols-4 gap-[14px]">
        {[0, 1, 2, 3].map(i => (
          <div key={i} className="bg-card border border-border rounded-[6px] h-[76px]" />
        ))}
      </div>
      <div className="bg-card border border-border rounded-[6px] h-[320px]" />
    </div>
  )
}

function ErrorBanner({ message, onRetry }: { message: string; onRetry: () => void }) {
  return (
    <div className="bg-card border border-neg/30 rounded-[6px] px-[16px] py-[14px] flex items-center gap-4">
      <span className="text-[12px] text-neg flex-1">{message}</span>
      <button
        onClick={onRetry}
        className="text-[11px] text-txt-2 hover:text-txt border border-border rounded px-3 py-1 transition-colors"
      >
        Retry
      </button>
    </div>
  )
}
