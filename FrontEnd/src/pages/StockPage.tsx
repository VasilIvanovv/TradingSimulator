import { useState, useEffect, useRef, type FormEvent } from 'react'
import { useParams, useNavigate } from 'react-router-dom'
import { createChart, ColorType } from 'lightweight-charts'
import { tradingApi, type PriceCandle } from '../api/trading'
import { ApiError } from '../api/client'
import { useAccount } from '../hooks/useAccount'
import { useSymbols } from '../hooks/useSymbols'
import { SymbolLogo } from '../components/SymbolLogo'

type Side  = 'buy' | 'sell'
type Range = '1M' | '3M' | '6M' | '1Y' | 'All'

// ── Range config ──────────────────────────────────────────────────────────────

function daysAgo(n: number): string {
  const d = new Date()
  d.setDate(d.getDate() - n)
  return d.toISOString().slice(0, 10)
}

const RANGES: { label: Range; startDate: () => string }[] = [
  { label: '1M',  startDate: () => daysAgo(30)  },
  { label: '3M',  startDate: () => daysAgo(90)  },
  { label: '6M',  startDate: () => daysAgo(180) },
  { label: '1Y',  startDate: () => daysAgo(365) },
  { label: 'All', startDate: () => '2024-01-01' },
]

// ── Chart ─────────────────────────────────────────────────────────────────────

function PriceChart({ candles }: { candles: PriceCandle[] }) {
  const containerRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    if (!containerRef.current || candles.length === 0) return

    const chart = createChart(containerRef.current, {
      autoSize: true,
      height: 280,
      layout: {
        background: { type: ColorType.Solid, color: 'transparent' },
        textColor: '#8B9AB0',
        fontSize: 11,
      },
      grid: {
        vertLines: { color: 'rgba(255,255,255,0.04)' },
        horzLines: { color: 'rgba(255,255,255,0.04)' },
      },
      timeScale:       { borderColor: 'rgba(255,255,255,0.08)' },
      rightPriceScale: { borderColor: 'rgba(255,255,255,0.08)' },
    })

    const series = chart.addCandlestickSeries({
      upColor:       '#26C6A6',
      downColor:     '#EF5350',
      borderVisible: false,
      wickUpColor:   '#26C6A6',
      wickDownColor: '#EF5350',
    })

    series.setData(candles.map(c => ({
      time:  c.timestamp as Parameters<typeof series.setData>[0][number]['time'],
      open:  c.open,
      high:  c.high,
      low:   c.low,
      close: c.close,
    })))

    chart.timeScale().fitContent()
    return () => chart.remove()
  }, [candles])

  return <div ref={containerRef} style={{ height: 280 }} className="w-full" />
}

// ── Page ──────────────────────────────────────────────────────────────────────

export function StockPage() {
  const { symbol = '' } = useParams<{ symbol: string }>()
  const navigate        = useNavigate()
  const { symbols }     = useSymbols()
  const { account, refresh: refreshAccount } = useAccount()

  const [range,        setRange]        = useState<Range>('6M')
  const [candles,      setCandles]      = useState<PriceCandle[]>([])
  const [chartLoading, setChartLoading] = useState(true)
  const [chartError,   setChartError]   = useState<string | null>(null)

  const [side,        setSide]        = useState<Side>('buy')
  const [amount,      setAmount]      = useState('')
  const [submitting,  setSubmitting]  = useState(false)
  const [orderError,  setOrderError]  = useState<string | null>(null)
  const [orderResult, setOrderResult] = useState<string | null>(null)

  const info        = symbols.find(s => s.symbol === symbol)
  const position    = account?.positions[symbol] ?? 0
  const lastCandle  = candles.at(-1)
  const prevCandle  = candles.at(-2)
  const marketPrice = lastCandle?.close ?? null
  const change      = lastCandle && prevCandle
    ? ((lastCandle.close - prevCandle.close) / prevCandle.close) * 100
    : null

  const parsedAmount  = Math.round(parseFloat(amount) * 100) / 100  // cents
  const quantity      = marketPrice && parsedAmount > 0
    ? Math.round((parsedAmount / marketPrice) * 1e6) / 1e6          // 6dp for shares
    : null

  // Fetch candles when symbol or range changes
  useEffect(() => {
    if (!symbol) return
    const rangeConfig = RANGES.find(r => r.label === range)!
    setChartLoading(true)
    setChartError(null)
    tradingApi.getHistory(symbol, '1day', rangeConfig.startDate())
      .then(data => setCandles(data.candles))
      .catch(err  => setChartError(err instanceof ApiError ? err.message : 'Failed to load price data'))
      .finally(() => setChartLoading(false))
  }, [symbol, range])

  async function handleOrder(e: FormEvent) {
    e.preventDefault()
    if (!marketPrice || !quantity) return
    setOrderError(null)
    setOrderResult(null)
    setSubmitting(true)
    try {
      const receipt = await tradingApi.placeOrder(symbol, side, quantity, marketPrice)
      setOrderResult(
        receipt.status === 'filled'
          ? `${side === 'buy' ? 'Bought' : 'Sold'} ${quantity.toFixed(6)} shares @ $${receipt.executionPrice.toFixed(2)}`
          : 'Order rejected')
      setAmount('')
      refreshAccount()
    } catch (err) {
      setOrderError(err instanceof ApiError ? err.message : 'Order failed')
    } finally {
      setSubmitting(false)
    }
  }

  return (
    <div className="max-w-3xl flex flex-col gap-[16px]">

      {/* Back link */}
      <button
        onClick={() => navigate('/markets')}
        className="self-start text-[11px] text-txt-3 hover:text-txt transition-colors flex items-center gap-[5px]"
      >
        <svg viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.5"
             className="w-[10px] h-[10px]">
          <polyline points="9,2 3,7 9,12"/>
        </svg>
        Markets
      </button>

      {/* Stock header */}
      <div className="flex items-center gap-[14px]">
        <SymbolLogo symbol={symbol} size={48} />
        <div className="flex-1 min-w-0">
          <div className="text-[18px] font-bold text-txt leading-tight">{info?.name ?? symbol}</div>
          <div className="flex items-center gap-[8px] mt-[3px]">
            <span className="text-[11px] font-mono text-txt-3">{symbol}</span>
            {info?.sector && (
              <span className="text-[10px] text-txt-3 bg-surface border border-border-dim px-[6px] py-[1px] rounded">
                {info.sector}
              </span>
            )}
          </div>
        </div>
        {lastCandle && (
          <div className="text-right shrink-0">
            <div className="text-[22px] font-mono font-bold text-txt tabular-nums">
              ${lastCandle.close.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}
            </div>
            {change !== null && (
              <div className={`text-[11px] font-mono ${change >= 0 ? 'text-pos' : 'text-neg'}`}>
                {change >= 0 ? '+' : ''}{change.toFixed(2)}% prev. close
              </div>
            )}
          </div>
        )}
      </div>

      {/* Chart */}
      <div className="bg-card border border-border rounded-[6px] px-[16px] pt-[14px] pb-[10px]">
        <div className="flex items-center justify-between mb-[12px]">
          <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold">
            Price History · Daily
          </div>
          {/* Range selector */}
          <div className="flex items-center gap-[2px]">
            {RANGES.map(r => (
              <button
                key={r.label}
                onClick={() => setRange(r.label)}
                className={
                  `px-[8px] py-[3px] rounded text-[10px] font-semibold transition-colors ` +
                  (range === r.label
                    ? 'bg-accent/20 text-accent'
                    : 'text-txt-3 hover:text-txt')
                }
              >
                {r.label}
              </button>
            ))}
          </div>
        </div>

        {chartLoading ? (
          <div className="h-[280px] flex items-center justify-center text-txt-3 text-[12px]">Loading chart…</div>
        ) : chartError ? (
          <div className="h-[280px] flex items-center justify-center text-neg text-[12px]">{chartError}</div>
        ) : candles.length === 0 ? (
          <div className="h-[280px] flex items-center justify-center text-txt-3 text-[12px]">No price data available</div>
        ) : (
          <PriceChart candles={candles} />
        )}
      </div>

      {/* Position + Order form */}
      <div className="grid grid-cols-2 gap-[14px]">

        {/* Position */}
        <div className="bg-card border border-border rounded-[6px] px-[16px] py-[14px]">
          <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold mb-[12px]">
            Your Position
          </div>
          {position > 0 ? (
            <>
              <div className="text-[24px] font-mono font-bold text-txt tabular-nums">{position}</div>
              <div className="text-[11px] text-txt-3 mt-[2px]">shares held</div>
              {lastCandle && (
                <div className="mt-[10px] text-[12px] font-mono text-txt-2">
                  ≈ ${(position * lastCandle.close).toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}
                </div>
              )}
            </>
          ) : (
            <div className="text-[12px] text-txt-3">No position in {symbol}.</div>
          )}
        </div>

        {/* Order form */}
        <div className="bg-card border border-border rounded-[6px] px-[16px] py-[14px]">
          <div className="text-[10px] uppercase tracking-[0.08em] text-txt-3 font-semibold mb-[12px]">
            Place Order
          </div>
          <form onSubmit={handleOrder} className="flex flex-col gap-[10px]">
            <div className="flex rounded overflow-hidden border border-border">
              <SideButton active={side === 'buy'}  label="Buy"  onClick={() => setSide('buy')}  color="pos" />
              <SideButton active={side === 'sell'} label="Sell" onClick={() => setSide('sell')} color="neg" />
            </div>

            {/* Market price pill */}
            <div className="flex items-center justify-between px-[10px] py-[6px] bg-surface rounded border border-border-dim">
              <span className="text-[10px] uppercase tracking-[0.07em] text-txt-3 font-semibold">Market price</span>
              <span className="text-[12px] font-mono tabular-nums text-txt">
                {marketPrice != null
                  ? `$${marketPrice.toLocaleString('en-US', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`
                  : '—'}
              </span>
            </div>

            {/* Amount input */}
            <div className="relative">
              <span className="absolute left-3 top-1/2 -translate-y-1/2 text-txt-3 text-[12px] pointer-events-none">$</span>
              <input
                type="number" min="0.01" step="0.01"
                value={amount}
                onChange={e => setAmount(e.target.value)}
                onBlur={e => {
                  const v = parseFloat(e.target.value)
                  if (!isNaN(v)) setAmount((Math.round(v * 100) / 100).toFixed(2))
                }}
                required placeholder="Amount to invest"
                className={inputCls + ' pl-6 font-mono tabular-nums'}
              />
            </div>

            {/* Derived quantity */}
            <div className="flex items-center justify-between px-[10px] py-[6px] bg-surface rounded border border-border-dim">
              <span className="text-[10px] uppercase tracking-[0.07em] text-txt-3 font-semibold">Shares</span>
              <span className="text-[12px] font-mono tabular-nums text-txt">
                {quantity != null ? quantity.toFixed(6) : '—'}
              </span>
            </div>

            {orderError  && <div className="text-neg text-[11px]">{orderError}</div>}
            {orderResult && <div className="text-pos text-[11px]">{orderResult}</div>}

            <button
              type="submit"
              disabled={submitting || !quantity}
              className={
                `py-[8px] rounded text-[12px] font-semibold transition-all disabled:opacity-50 disabled:cursor-not-allowed ` +
                (side === 'buy' ? 'bg-pos text-[#050C18] hover:brightness-110' : 'bg-neg text-white hover:brightness-110')
              }
            >
              {submitting ? 'Placing…' : `${side === 'buy' ? 'Buy' : 'Sell'} ${symbol}`}
            </button>
          </form>
        </div>
      </div>
    </div>
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

const inputCls =
  'bg-surface border border-border rounded px-3 py-[7px] text-txt text-[12px] outline-none ' +
  'focus:border-accent transition-colors placeholder:text-txt-3 w-full'
