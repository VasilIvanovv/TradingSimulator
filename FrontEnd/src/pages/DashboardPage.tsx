import { useState, useEffect, type ReactNode } from 'react'
import { useAccount } from '../hooks/useAccount'
import { tradingApi, type AccountSnapshot, type TradeRecord } from '../api/trading'

// ── Constants ─────────────────────────────────────────────────────────────────

const INITIAL_CASH = 10_000

type Period = '1D' | '1W' | '1M' | '3M' | '1Y' | 'ALL'
const PERIODS: Period[] = ['1D', '1W', '1M', '3M', '1Y', 'ALL']
const PERIOD_DAYS: Record<string, number> = { '1D': 1, '1W': 7, '1M': 30, '3M': 90, '1Y': 365 }

// ── Helpers ───────────────────────────────────────────────────────────────────

function daysAgo(n: number) {
  const d = new Date()
  d.setDate(d.getDate() - n)
  return d.toISOString().slice(0, 10)
}

// ── Current price cache — 1-hour candles ──────────────────────────────────────

const PRICE_TTL_MS = 60 * 60 * 1000

let priceCache: {
  prices:    Record<string, number>
  timestamp: number
  symbolKey: string
} | null = null

function usePositionPrices(symbols: string[]) {
  const key = symbols.slice().sort().join(',')

  const cacheValid = priceCache !== null &&
                     priceCache.symbolKey === key &&
                     Date.now() - priceCache.timestamp < PRICE_TTL_MS

  const [prices,    setPrices]    = useState<Record<string, number>>(cacheValid ? priceCache!.prices : {})
  const [loading,   setLoading]   = useState(!cacheValid && symbols.length > 0)
  const [updatedAt, setUpdatedAt] = useState<Date | null>(cacheValid ? new Date(priceCache!.timestamp) : null)

  useEffect(() => {
    if (symbols.length === 0) { setPrices({}); return }
    if (cacheValid) return

    setLoading(true)
    Promise.all(
      symbols.map(s =>
        tradingApi.getHistory(s, '1h', daysAgo(3))
          .then(data => ({ s, price: data.candles.at(-1)?.close ?? null }))
          .catch(()  => ({ s, price: null }))
      )
    ).then(results => {
      const map: Record<string, number> = {}
      for (const r of results)
        if (r.price !== null) map[r.s] = r.price
      priceCache = { prices: map, timestamp: Date.now(), symbolKey: key }
      setPrices(map)
      setUpdatedAt(new Date())
    }).finally(() => setLoading(false))
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [key])

  return { prices, pricesLoading: loading, updatedAt }
}

// ── Period P&L — reconstructs full portfolio at period start from trade history ─

const histPriceCache: Record<string, { prices: Record<string, number>; ts: number }> = {}

interface PastPortfolio {
  cash:      number
  positions: Record<string, number>
  prices:    Record<string, number>  // historical prices for those positions
}

function usePeriodPnl(
  period: Period,
  trades: TradeRecord[],
  currentPositions: Record<string, number>,
  currentCash: number,
  currentPrices: Record<string, number>,
  pricesReady: boolean,
) {
  const [past,    setPast]    = useState<PastPortfolio | null>(null)
  const [loading, setLoading] = useState(false)

  // Stable key so the effect only re-runs when trades actually change.
  const tradesKey = trades.map(t => `${t.timestamp}${t.symbol}${t.side}${t.quantity}`).join('|')

  useEffect(() => {
    if (period === 'ALL') { setPast(null); setLoading(false); return }

    const periodStart = daysAgo(PERIOD_DAYS[period])

    // Reconstruct portfolio state at period start from trade history (synchronous).
    // Trades with no timestamp (empty string) are treated as pre-dating all periods.
    let pastCash = INITIAL_CASH
    const pastPositions: Record<string, number> = {}
    for (const t of trades) {
      const tDate = t.timestamp.slice(0, 10)
      if (tDate !== '' && tDate >= periodStart) continue  // trade happened during/after period
      if (t.side === 'buy') {
        pastCash -= t.quantity * t.executionPrice
        pastPositions[t.symbol] = (pastPositions[t.symbol] ?? 0) + t.quantity
      } else {
        pastCash += t.quantity * t.executionPrice
        const newQty = (pastPositions[t.symbol] ?? 0) - t.quantity
        if (newQty <= 0) delete pastPositions[t.symbol]
        else pastPositions[t.symbol] = newQty
      }
    }

    const pastSymbols = Object.keys(pastPositions).sort()

    if (pastSymbols.length === 0) {
      setPast({ cash: pastCash, positions: {}, prices: {} })
      setLoading(false)
      return
    }

    const cacheKey = `${period}:${pastSymbols.join(',')}`
    const hit = histPriceCache[cacheKey]
    if (hit && Date.now() - hit.ts < PRICE_TTL_MS) {
      setPast({ cash: pastCash, positions: pastPositions, prices: hit.prices })
      setLoading(false)
      return
    }

    setLoading(true)
    const fetchFrom  = daysAgo(PERIOD_DAYS[period] + 5) // 5-day buffer for weekends/holidays

    Promise.all(
      pastSymbols.map(s =>
        tradingApi.getHistory(s, '1day', fetchFrom)
          .then(data => {
            // Most recent close on or before the period-start date.
            const c = data.candles.filter(c => c.timestamp.slice(0, 10) <= periodStart).at(-1)
                   ?? data.candles.at(0)
            return { s, price: c?.close ?? null }
          })
          .catch(() => ({ s, price: null as number | null }))
      )
    ).then(results => {
      const prices: Record<string, number> = {}
      for (const { s, price } of results)
        if (price !== null) prices[s] = price
      histPriceCache[cacheKey] = { prices, ts: Date.now() }
      setPast({ cash: pastCash, positions: pastPositions, prices })
      setLoading(false)
    })
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [period, tradesKey])

  if (!pricesReady) return { pnlPct: null as number | null, pnlAbs: null as number | null, pnlLoading: false }

  const currentTotal = currentCash +
    Object.entries(currentPositions).reduce((s, [sym, q]) => s + q * (currentPrices[sym] ?? 0), 0)

  if (period === 'ALL') {
    const pnlAbs = currentTotal - INITIAL_CASH
    return { pnlPct: (pnlAbs / INITIAL_CASH) * 100, pnlAbs, pnlLoading: false }
  }

  if (loading || past === null) return { pnlPct: null as number | null, pnlAbs: null as number | null, pnlLoading: loading }

  if (!Object.keys(past.positions).every(s => past.prices[s] != null)) {
    return { pnlPct: null as number | null, pnlAbs: null as number | null, pnlLoading: false }
  }

  const pastPositionsValue = Object.entries(past.positions)
    .reduce((s, [sym, q]) => s + q * past.prices[sym], 0)
  const pastTotal = past.cash + pastPositionsValue

  if (pastTotal === 0) return { pnlPct: null as number | null, pnlAbs: null as number | null, pnlLoading: false }

  const pnlAbs = currentTotal - pastTotal
  return { pnlPct: (pnlAbs / pastTotal) * 100, pnlAbs, pnlLoading: false }
}

// ── Page ──────────────────────────────────────────────────────────────────────

export function DashboardPage() {
  const [period, setPeriod] = useState<Period>('ALL')

  const { account, loading, error, refresh } = useAccount()
  const symbols = account ? Object.keys(account.positions) : []
  const { prices, pricesLoading, updatedAt } = usePositionPrices(symbols)

  const totalInvested = account
    ? Object.entries(account.positions).reduce((sum, [sym, qty]) => {
        const price = prices[sym]
        return price != null ? sum + qty * price : sum
      }, 0)
    : 0
  const totalValue  = account ? account.cash + totalInvested : 0
  const pricesReady = !loading && !pricesLoading && symbols.every(s => prices[s] != null)

  const { pnlPct, pnlAbs, pnlLoading } = usePeriodPnl(
    period,
    account?.tradeHistory ?? [],
    account?.positions ?? {},
    account?.cash ?? 0,
    prices,
    pricesReady,
  )

  if (loading) return <Skeleton />
  if (error)   return <ErrorBanner message={error} onRetry={refresh} />

  const acc = account!

  return (
    <div className="flex flex-col gap-[14px]">
      {/* Summary strip */}
      <div className="grid grid-cols-4 gap-[14px]">
        <StatCard label="Cash"         value={`$${fmt(acc.cash)}`} />
        <StatCard label="Total Value"  value={pricesReady ? `$${fmt(totalValue)}` : '…'} dim={!pricesReady} />
        <StatCard label="Total Trades" value={String(acc.tradeHistory.length)} />
        <PerformanceCard pnlPct={pnlPct} pnlAbs={pnlAbs} loading={pnlLoading} period={period} onPeriodChange={setPeriod} />
      </div>

      <div className="grid grid-cols-2 gap-[14px]">
        <PositionsCard account={acc} prices={prices} pricesLoading={pricesLoading} updatedAt={updatedAt} />
        <div className="flex flex-col gap-[14px]">
          <AllocationCard positions={acc.positions} prices={prices} cash={acc.cash} pricesLoading={pricesLoading} />
          <RecentTradesCard trades={acc.tradeHistory.slice(-8).reverse()} />
        </div>
      </div>
    </div>
  )
}

// ── Summary strip ─────────────────────────────────────────────────────────────

function StatCard({ label, value, dim }: { label: string; value: string; dim?: boolean }) {
  return (
    <div className="bg-card border border-border rounded-[6px] px-[16px] py-[14px]">
      <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold mb-[6px]">{label}</div>
      <div className={`text-[22px] font-mono font-semibold tabular-nums ${dim ? 'text-txt-3' : 'text-txt'}`}>
        {value}
      </div>
    </div>
  )
}

function PerformanceCard({
  pnlPct, pnlAbs, loading, period, onPeriodChange,
}: {
  pnlPct: number | null
  pnlAbs: number | null
  loading: boolean
  period: Period
  onPeriodChange: (p: Period) => void
}) {
  const positive = pnlPct != null && pnlPct >= 0
  const color = pnlPct == null
    ? 'text-txt-3'
    : positive ? 'text-pos' : 'text-neg'

  const sign = positive ? '+' : ''

  const pctDisplay = loading ? '…' : pnlPct == null ? '—' : `${sign}${pnlPct.toFixed(2)}%`
  const absDisplay = loading || pnlAbs == null ? null : `${sign}$${fmt(Math.abs(pnlAbs))}`

  return (
    <div className="bg-card border border-border rounded-[6px] px-[16px] py-[14px] flex flex-col">
      <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold mb-[6px]">Return</div>
      <div className="flex items-baseline gap-[8px]">
        <div className={`text-[22px] font-mono font-semibold tabular-nums ${color}`}>
          {pctDisplay}
        </div>
        {absDisplay && (
          <div className={`text-[12px] font-mono tabular-nums ${color}`}>
            {absDisplay}
          </div>
        )}
      </div>
      <div className="flex gap-[3px] mt-auto pt-[8px]">
        {PERIODS.map(p => (
          <button
            key={p}
            onClick={() => onPeriodChange(p)}
            className={`text-[9px] font-semibold uppercase tracking-wide px-[5px] py-[2px] rounded-[3px] transition-colors ${
              p === period
                ? 'text-txt bg-white/10'
                : 'text-txt-3 hover:text-txt-2'
            }`}
          >
            {p}
          </button>
        ))}
      </div>
    </div>
  )
}

// ── Positions ─────────────────────────────────────────────────────────────────

// Average-cost method: computes cost basis per share for every current position.
function computeAvgCosts(trades: TradeRecord[]): Record<string, number> {
  const cost: Record<string, number> = {}
  const qty:  Record<string, number> = {}

  for (const t of trades) {
    if (t.side === 'buy') {
      cost[t.symbol] = (cost[t.symbol] ?? 0) + t.quantity * t.executionPrice
      qty[t.symbol]  = (qty[t.symbol]  ?? 0) + t.quantity
    } else {
      const avgCost  = (cost[t.symbol] ?? 0) / (qty[t.symbol] ?? 1)
      qty[t.symbol]  = (qty[t.symbol]  ?? 0) - t.quantity
      cost[t.symbol] = Math.max(0, qty[t.symbol]) * avgCost
    }
  }

  const result: Record<string, number> = {}
  for (const sym of Object.keys(qty))
    if (qty[sym] > 0) result[sym] = cost[sym] / qty[sym]
  return result
}

function PositionsCard({
  account, prices, pricesLoading, updatedAt,
}: {
  account: AccountSnapshot
  prices: Record<string, number>
  pricesLoading: boolean
  updatedAt: Date | null
}) {
  const entries  = Object.entries(account.positions)
  const avgCosts = computeAvgCosts(account.tradeHistory)

  const allPriced = entries.length > 0 && entries.every(([s]) => prices[s] != null)

  const totalMarketValue = entries.reduce((sum, [sym, qty]) => {
    const price = prices[sym]
    return price != null ? sum + qty * price : sum
  }, 0)

  const totalPnl = allPriced
    ? entries.reduce((sum, [sym, qty]) => {
        const price   = prices[sym]
        const avgCost = avgCosts[sym]
        return price != null && avgCost != null ? sum + qty * (price - avgCost) : sum
      }, 0)
    : null

  const updatedLabel = pricesLoading
    ? 'Loading prices…'
    : updatedAt
      ? `Prices as of ${updatedAt.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })} · hourly`
      : null

  return (
    <Card title="Positions" count={entries.length} subtitle={updatedLabel}>
      {entries.length === 0 ? (
        <Empty>No open positions</Empty>
      ) : (
        <table className="w-full text-[12px]">
          <thead>
            <tr className="text-txt-3 text-[10px] uppercase tracking-[0.07em] border-b border-border-dim">
              <th className="text-left  py-[6px] font-semibold">Symbol</th>
              <th className="text-right py-[6px] font-semibold">Qty</th>
              <th className="text-right py-[6px] font-semibold">Avg Cost</th>
              <th className="text-right py-[6px] font-semibold">Price</th>
              <th className="text-right py-[6px] font-semibold">P&amp;L</th>
              <th className="text-right py-[6px] font-semibold">Value</th>
            </tr>
          </thead>
          <tbody>
            {entries.map(([symbol, qty]) => {
              const price   = prices[symbol]
              const avgCost = avgCosts[symbol]
              const pnl     = price != null && avgCost != null ? qty * (price - avgCost) : null
              const pnlPct  = price != null && avgCost != null ? ((price - avgCost) / avgCost) * 100 : null
              const pos     = pnl != null && pnl >= 0
              const pnlColor = pnl == null ? 'text-txt-3' : pos ? 'text-pos' : 'text-neg'

              return (
                <tr key={symbol} className="border-b border-border-dim last:border-0">
                  <td className="py-[7px] font-mono text-txt">{symbol}</td>
                  <td className="py-[7px] text-right font-mono tabular-nums text-txt-2">
                    {qty.toFixed(6).replace(/\.?0+$/, '')}
                  </td>
                  <td className="py-[7px] text-right font-mono tabular-nums text-txt-2">
                    {avgCost != null ? `$${fmt(avgCost)}` : '—'}
                  </td>
                  <td className="py-[7px] text-right font-mono tabular-nums text-txt-2">
                    {pricesLoading ? <Dot /> : price != null ? `$${fmt(price)}` : '—'}
                  </td>
                  <td className={`py-[7px] text-right font-mono tabular-nums ${pnlColor}`}>
                    {pricesLoading ? <Dot /> : pnl == null ? '—' : (
                      <div>
                        <div>{pos ? '+' : ''}{fmt(pnl)}</div>
                        <div className="text-[10px] opacity-80">{pos ? '+' : ''}{pnlPct!.toFixed(2)}%</div>
                      </div>
                    )}
                  </td>
                  <td className="py-[7px] text-right font-mono tabular-nums text-txt">
                    {pricesLoading ? <Dot /> : price != null ? `$${fmt(qty * price)}` : '—'}
                  </td>
                </tr>
              )
            })}
          </tbody>
          {allPriced && (
            <tfoot>
              <tr className="border-t border-border text-txt-3">
                <td colSpan={4} className="pt-[8px] text-[10px] uppercase tracking-[0.07em] font-semibold">
                  Market Value
                </td>
                <td className={`pt-[8px] text-right font-mono tabular-nums text-[11px] font-semibold ${
                  totalPnl == null ? 'text-txt-3' : totalPnl >= 0 ? 'text-pos' : 'text-neg'
                }`}>
                  {totalPnl != null && `${totalPnl >= 0 ? '+' : ''}${fmt(totalPnl)}`}
                </td>
                <td className="pt-[8px] text-right font-mono tabular-nums text-txt font-semibold">
                  ${fmt(totalMarketValue)}
                </td>
              </tr>
            </tfoot>
          )}
        </table>
      )}
    </Card>
  )
}

// ── Allocation chart ──────────────────────────────────────────────────────────

const CHART_PALETTE = ['#4ade80','#60a5fa','#f59e0b','#a78bfa','#fb923c','#34d399','#e879f9','#38bdf8']
const DONUT_R    = 35
const DONUT_CIRC = 2 * Math.PI * DONUT_R

function AllocationCard({
  positions, prices, cash, pricesLoading,
}: {
  positions:     Record<string, number>
  prices:        Record<string, number>
  cash:          number
  pricesLoading: boolean
}) {
  const posEntries = Object.entries(positions).filter(([sym]) => prices[sym] != null)
  const segments = [
    ...posEntries.map(([sym, qty], i) => ({
      label: sym,
      value: qty * prices[sym],
      color: CHART_PALETTE[i % CHART_PALETTE.length],
    })),
    ...(cash > 0 ? [{ label: 'Cash', value: cash, color: '#6b7280' }] : []),
  ]
  const total = segments.reduce((s, seg) => s + seg.value, 0)

  if (pricesLoading || total === 0)
    return <Card title="Allocation"><Empty>{pricesLoading ? 'Loading…' : 'No positions'}</Empty></Card>

  let accumulated = 0
  const slices = segments.map(seg => {
    const frac  = seg.value / total
    const dash  = Math.max(0, frac * DONUT_CIRC - 1.5)
    const slice = { ...seg, frac, dasharray: `${dash} ${DONUT_CIRC - dash}`, dashoffset: -accumulated * DONUT_CIRC }
    accumulated += frac
    return slice
  })

  return (
    <Card title="Allocation">
      <div className="flex items-center gap-[20px]">
        <svg viewBox="0 0 100 100" width="90" height="90" className="shrink-0">
          <circle cx="50" cy="50" r={DONUT_R} fill="none"
            stroke="currentColor" strokeOpacity="0.08" strokeWidth="14" />
          {slices.map((s, i) => (
            <circle key={i} cx="50" cy="50" r={DONUT_R} fill="none"
              stroke={s.color} strokeWidth="14"
              strokeDasharray={s.dasharray}
              strokeDashoffset={s.dashoffset}
              transform="rotate(-90 50 50)" />
          ))}
          <text x="50" y="46" textAnchor="middle" fontSize="7" fill="currentColor" fillOpacity="0.4">
            assets
          </text>
          <text x="50" y="58" textAnchor="middle" fontSize="12" fontWeight="600"
            fill="currentColor" fillOpacity="0.75">
            {segments.length}
          </text>
        </svg>

        <div className="flex flex-col gap-[6px] min-w-0 flex-1">
          {slices.map((s, i) => (
            <div key={i} className="flex items-center gap-[6px] min-w-0">
              <div className="w-[7px] h-[7px] rounded-full shrink-0" style={{ background: s.color }} />
              <span className="font-mono text-[11px] text-txt-2 shrink-0 w-[42px]">{s.label}</span>
              <div className="flex-1 bg-white/5 rounded-full h-[3px] min-w-0">
                <div className="h-[3px] rounded-full" style={{ width: `${s.frac * 100}%`, background: s.color, opacity: 0.55 }} />
              </div>
              <span className="font-mono text-[10px] text-txt-3 tabular-nums shrink-0 w-[36px] text-right">
                {(s.frac * 100).toFixed(1)}%
              </span>
            </div>
          ))}
        </div>
      </div>
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

function Card({ title, count, subtitle, children }: { title: string; count?: number; subtitle?: string | null; children: ReactNode }) {
  return (
    <div className="bg-card border border-border rounded-[6px] flex flex-col">
      <div className="flex items-center justify-between px-[16px] py-[12px] border-b border-border-dim">
        <div className="flex items-baseline gap-[8px] min-w-0">
          <span className="text-[11px] font-semibold uppercase tracking-[0.08em] text-txt-2 shrink-0">{title}</span>
          {subtitle && <span className="text-[10px] text-txt-3 truncate">{subtitle}</span>}
        </div>
        {count !== undefined && (
          <span className="text-[10px] font-mono text-txt-3 shrink-0 ml-2">{count}</span>
        )}
      </div>
      <div className="px-[16px] py-[10px] flex-1 overflow-x-auto">{children}</div>
    </div>
  )
}

function Empty({ children }: { children: ReactNode }) {
  return (
    <div className="flex items-center justify-center py-[24px] text-[12px] text-txt-3">{children}</div>
  )
}

function Dot() {
  return <span className="text-txt-3">…</span>
}

function Skeleton() {
  return (
    <div className="flex flex-col gap-[14px] animate-pulse">
      <div className="grid grid-cols-4 gap-[14px]">
        {[0, 1, 2, 3].map(i => (
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

function fmt(n: number) {
  return n.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })
}
