import { type ReactNode } from 'react'
import { useAccount } from '../hooks/useAccount'
import { type AccountSnapshot, type TradeRecord } from '../api/trading'

export function DashboardPage() {
  const { account, loading, error, refresh } = useAccount()

  if (loading) return <Skeleton />
  if (error)   return <ErrorBanner message={error} onRetry={refresh} />

  return (
    <div className="flex flex-col gap-[14px]">
      <SummaryRow account={account!} />
      <div className="grid grid-cols-2 gap-[14px]">
        <PositionsCard account={account!} />
        <RecentTradesCard trades={account!.tradeHistory.slice(-8).reverse()} />
      </div>
    </div>
  )
}

// ── Summary strip ─────────────────────────────────────────────────────────────

function SummaryRow({ account }: { account: AccountSnapshot }) {
  const posCount = Object.keys(account.positions).length
  return (
    <div className="grid grid-cols-3 gap-[14px]">
      <StatCard label="Available Cash"   value={`$${fmt(account.cash)}`} />
      <StatCard label="Open Positions"   value={String(posCount)} />
      <StatCard label="Total Trades"     value={String(account.tradeHistory.length)} />
    </div>
  )
}

function StatCard({ label, value }: { label: string; value: string }) {
  return (
    <div className="bg-card border border-border rounded-[6px] px-[16px] py-[14px]">
      <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold mb-[6px]">{label}</div>
      <div className="text-[22px] font-mono font-semibold tabular-nums text-txt">{value}</div>
    </div>
  )
}

// ── Positions ─────────────────────────────────────────────────────────────────

function PositionsCard({ account }: { account: AccountSnapshot }) {
  const entries = Object.entries(account.positions)
  return (
    <Card title="Positions" count={entries.length}>
      {entries.length === 0 ? (
        <Empty>No open positions</Empty>
      ) : (
        <table className="w-full text-[12px]">
          <thead>
            <tr className="text-txt-3 text-[10px] uppercase tracking-[0.07em] border-b border-border-dim">
              <th className="text-left py-[6px] font-semibold">Symbol</th>
              <th className="text-right py-[6px] font-semibold">Qty</th>
            </tr>
          </thead>
          <tbody>
            {entries.map(([symbol, qty]) => (
              <tr key={symbol} className="border-b border-border-dim last:border-0">
                <td className="py-[7px] text-txt font-mono">{symbol}</td>
                <td className="py-[7px] text-right font-mono tabular-nums text-txt">{qty}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </Card>
  )
}

// ── Recent trades ─────────────────────────────────────────────────────────────

function RecentTradesCard({ trades }: { trades: TradeRecord[] }) {
  return (
    <Card title="Recent Trades" count={trades.length}>
      {trades.length === 0 ? (
        <Empty>No trades yet</Empty>
      ) : (
        <table className="w-full text-[12px]">
          <thead>
            <tr className="text-txt-3 text-[10px] uppercase tracking-[0.07em] border-b border-border-dim">
              <th className="text-left  py-[6px] font-semibold">Symbol</th>
              <th className="text-left  py-[6px] font-semibold">Side</th>
              <th className="text-right py-[6px] font-semibold">Qty</th>
              <th className="text-right py-[6px] font-semibold">Price</th>
            </tr>
          </thead>
          <tbody>
            {trades.map((t, i) => (
              <tr key={i} className="border-b border-border-dim last:border-0">
                <td className="py-[7px] font-mono text-txt">{t.symbol}</td>
                <td className="py-[7px]">
                  <span className={`text-[10px] font-semibold uppercase tracking-wide ${t.side === 'buy' ? 'text-pos' : 'text-neg'}`}>
                    {t.side}
                  </span>
                </td>
                <td className="py-[7px] text-right font-mono tabular-nums text-txt">{t.quantity}</td>
                <td className="py-[7px] text-right font-mono tabular-nums text-txt">${fmt(t.executionPrice)}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </Card>
  )
}

// ── Primitives ────────────────────────────────────────────────────────────────

function Card({ title, count, children }: { title: string; count?: number; children: ReactNode }) {
  return (
    <div className="bg-card border border-border rounded-[6px] flex flex-col">
      <div className="flex items-center justify-between px-[16px] py-[12px] border-b border-border-dim">
        <span className="text-[11px] font-semibold uppercase tracking-[0.08em] text-txt-2">{title}</span>
        {count !== undefined && (
          <span className="text-[10px] font-mono text-txt-3">{count}</span>
        )}
      </div>
      <div className="px-[16px] py-[10px] flex-1">{children}</div>
    </div>
  )
}

function Empty({ children }: { children: ReactNode }) {
  return (
    <div className="flex items-center justify-center py-[24px] text-[12px] text-txt-3">{children}</div>
  )
}

function Skeleton() {
  return (
    <div className="flex flex-col gap-[14px] animate-pulse">
      <div className="grid grid-cols-3 gap-[14px]">
        {[0, 1, 2].map(i => (
          <div key={i} className="bg-card border border-border rounded-[6px] h-[76px]" />
        ))}
      </div>
      <div className="grid grid-cols-2 gap-[14px]">
        <div className="bg-card border border-border rounded-[6px] h-[220px]" />
        <div className="bg-card border border-border rounded-[6px] h-[220px]" />
      </div>
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

// ── Util ──────────────────────────────────────────────────────────────────────

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}
