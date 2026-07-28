import { useState, useEffect, useRef, type ReactNode } from 'react'
import { useSymbols } from '../hooks/useSymbols'
import { type SymbolInfo } from '../api/trading'
import { BASE } from '../api/client'

interface Props {
  value:    string
  onChange: (symbol: string) => void
}

// Deterministic avatar background color from the ticker's first character.
const AVATAR_COLORS = [
  '#1A3A5C', '#1A4A3A', '#3A1A4A', '#4A2A1A',
  '#1A3A4A', '#3A3A1A', '#1A1A4A', '#3A1A1A',
]
function avatarColor(symbol: string): string {
  return AVATAR_COLORS[symbol.charCodeAt(0) % AVATAR_COLORS.length]
}

export function SymbolPicker({ value, onChange }: Props) {
  const [open, setOpen]     = useState(false)
  const [search, setSearch] = useState('')
  const { symbols, loading } = useSymbols()
  const searchRef = useRef<HTMLInputElement>(null)

  // Auto-focus the search input when picker opens.
  useEffect(() => {
    if (open) setTimeout(() => searchRef.current?.focus(), 50)
    else setSearch('')
  }, [open])

  // Close on ESC.
  useEffect(() => {
    if (!open) return
    const handler = (e: KeyboardEvent) => { if (e.key === 'Escape') setOpen(false) }
    window.addEventListener('keydown', handler)
    return () => window.removeEventListener('keydown', handler)
  }, [open])

  function select(symbol: string) {
    onChange(symbol)
    setOpen(false)
  }

  // Group symbols by sector when not searching; flat list when searching.
  const filtered = search.trim()
    ? symbols.filter(s =>
        s.symbol.toLowerCase().includes(search.toLowerCase()) ||
        s.name.toLowerCase().includes(search.toLowerCase()))
    : null

  const grouped: Record<string, SymbolInfo[]> = {}
  if (!filtered) {
    for (const s of symbols) {
      const sector = s.sector || 'Other'
      if (!grouped[sector]) grouped[sector] = []
      grouped[sector].push(s)
    }
  }

  const selected = symbols.find(s => s.symbol === value)

  return (
    <>
      {/* Trigger */}
      <button
        type="button"
        onClick={() => setOpen(true)}
        className="w-full flex items-center gap-[10px] bg-surface border border-border rounded
                   px-3 py-[7px] text-left hover:border-accent transition-colors outline-none
                   focus:border-accent"
      >
        {value ? (
          <>
            <Logo symbol={value} size={22} />
            <span className="text-txt text-[12px] font-medium flex-1">{value}</span>
            <span className="text-txt-3 text-[11px]">{selected?.name}</span>
          </>
        ) : (
          <span className="text-txt-3 text-[12px]">Select a stock or ETF…</span>
        )}
      </button>

      {/* Modal overlay */}
      {open && (
        <div
          className="fixed inset-0 z-50 flex items-center justify-center"
          onClick={() => setOpen(false)}
        >
          {/* Backdrop */}
          <div className="absolute inset-0 bg-black/60" />

          {/* Panel */}
          <div
            className="relative bg-surface border border-border rounded-[8px] w-[360px] max-h-[70vh]
                       flex flex-col shadow-2xl"
            onClick={e => e.stopPropagation()}
          >
            {/* Search */}
            <div className="p-[12px] border-b border-border-dim">
              <div className="flex items-center gap-[8px] bg-card border border-border rounded px-3 py-[7px]">
                <SearchIcon />
                <input
                  ref={searchRef}
                  type="text"
                  value={search}
                  onChange={e => setSearch(e.target.value)}
                  placeholder="Search stocks and ETFs…"
                  className="flex-1 bg-transparent outline-none text-txt text-[12px] placeholder:text-txt-3"
                />
                {search && (
                  <button onClick={() => setSearch('')} className="text-txt-3 hover:text-txt text-[14px]">×</button>
                )}
              </div>
            </div>

            {/* List */}
            <div className="flex-1 overflow-y-auto">
              {loading ? (
                <div className="flex items-center justify-center py-10 text-txt-3 text-[12px]">
                  Loading…
                </div>
              ) : filtered ? (
                filtered.length === 0 ? (
                  <div className="flex items-center justify-center py-10 text-txt-3 text-[12px]">
                    No results for "{search}"
                  </div>
                ) : (
                  filtered.map(s => (
                    <SymbolRow key={s.symbol} info={s} onSelect={select} />
                  ))
                )
              ) : (
                Object.entries(grouped).map(([sector, items]) => (
                  <div key={sector}>
                    <div className="px-[14px] py-[6px] text-[10px] uppercase tracking-[0.09em]
                                    text-txt-3 font-semibold bg-bg sticky top-0">
                      {sector}
                    </div>
                    {items.map(s => (
                      <SymbolRow key={s.symbol} info={s} onSelect={select} />
                    ))}
                  </div>
                ))
              )}
            </div>
          </div>
        </div>
      )}
    </>
  )
}

// ── Row ───────────────────────────────────────────────────────────────────────

function SymbolRow({ info, onSelect }: { info: SymbolInfo; onSelect: (s: string) => void }) {
  return (
    <button
      type="button"
      onClick={() => onSelect(info.symbol)}
      className="w-full flex items-center gap-[12px] px-[14px] py-[10px]
                 hover:bg-card transition-colors border-b border-border-dim last:border-0"
    >
      <Logo symbol={info.symbol} size={32} />
      <div className="flex-1 min-w-0 text-left">
        <div className="text-[12.5px] font-medium text-txt truncate">{info.name}</div>
        <div className="text-[10px] text-txt-3 font-mono">{info.symbol}</div>
      </div>
    </button>
  )
}

// ── Logo ──────────────────────────────────────────────────────────────────────

function Logo({ symbol, size }: { symbol: string; size: number }) {
  const [failed, setFailed] = useState(false)
  const style = { width: size, height: size, minWidth: size }

  if (failed) return <LetterAvatar symbol={symbol} size={size} />

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

function LetterAvatar({ symbol, size }: { symbol: string; size: number }) {
  return (
    <div
      style={{
        width: size, height: size, minWidth: size,
        background: avatarColor(symbol),
        fontSize: Math.round(size * 0.38),
      }}
      className="rounded-full flex items-center justify-center font-mono font-bold text-txt-2"
    >
      {symbol[0]}
    </div>
  )
}

function SearchIcon() {
  return (
    <svg viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.4"
         className="w-[13px] h-[13px] text-txt-3 shrink-0">
      <circle cx="6" cy="6" r="4.5"/>
      <line x1="9.5" y1="9.5" x2="13" y2="13"/>
    </svg>
  )
}

// Logo state resets when the symbol prop changes (e.g. after clearing selection).
// Achieved by keying on symbol in the parent — not needed here since Logo is
// always rendered fresh from SymbolRow with a stable symbol prop.
export type { Props as SymbolPickerProps }
