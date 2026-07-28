import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { useSymbols } from '../hooks/useSymbols'
import { type SymbolInfo } from '../api/trading'
import { SymbolLogo } from '../components/SymbolLogo'

export function MarketsPage() {
  const { symbols, loading } = useSymbols()
  const navigate = useNavigate()
  const [search, setSearch] = useState('')

  const filtered = search.trim()
    ? symbols.filter(s =>
        s.symbol.toLowerCase().includes(search.toLowerCase()) ||
        s.name.toLowerCase().includes(search.toLowerCase()))
    : null

  const grouped: Record<string, SymbolInfo[]> = {}
  if (!filtered) {
    for (const s of symbols) {
      if (!grouped[s.sector]) grouped[s.sector] = []
      grouped[s.sector].push(s)
    }
  }

  return (
    <div className="max-w-xl flex flex-col gap-[14px]">

      {/* Search */}
      <div className="flex items-center gap-[10px] bg-card border border-border rounded px-3 py-[9px]">
        <SearchIcon />
        <input
          type="text"
          value={search}
          onChange={e => setSearch(e.target.value)}
          placeholder="Search stocks and ETFs…"
          className="flex-1 bg-transparent outline-none text-txt text-[13px] placeholder:text-txt-3"
          autoFocus
        />
        {search && (
          <button onClick={() => setSearch('')} className="text-txt-3 hover:text-txt text-[16px]">×</button>
        )}
      </div>

      {/* List */}
      <div className="bg-card border border-border rounded-[6px] overflow-hidden">
        {loading ? (
          <div className="divide-y divide-border-dim">
            {[...Array(6)].map((_, i) => (
              <div key={i} className="px-[16px] py-[12px] flex items-center gap-[12px]">
                <div className="w-9 h-9 rounded-full bg-surface animate-pulse shrink-0" />
                <div className="flex flex-col gap-[5px] flex-1">
                  <div className="h-[12px] w-32 bg-surface rounded animate-pulse" />
                  <div className="h-[9px] w-12 bg-surface rounded animate-pulse" />
                </div>
              </div>
            ))}
          </div>
        ) : filtered ? (
          filtered.length === 0 ? (
            <div className="py-10 text-center text-txt-3 text-[12px]">No results for "{search}"</div>
          ) : (
            <div className="divide-y divide-border-dim">
              {filtered.map(s => (
                <StockRow key={s.symbol} info={s} onClick={() => navigate(`/stocks/${s.symbol}`)} />
              ))}
            </div>
          )
        ) : (
          Object.entries(grouped).map(([sector, items]) => (
            <div key={sector}>
              <div className="px-[16px] py-[6px] text-[10px] uppercase tracking-[0.09em] text-txt-3 font-semibold bg-surface sticky top-0 border-b border-border-dim">
                {sector}
              </div>
              <div className="divide-y divide-border-dim">
                {items.map(s => (
                  <StockRow key={s.symbol} info={s} onClick={() => navigate(`/stocks/${s.symbol}`)} />
                ))}
              </div>
            </div>
          ))
        )}
      </div>
    </div>
  )
}

// ── Row ───────────────────────────────────────────────────────────────────────

function StockRow({ info, onClick }: { info: SymbolInfo; onClick: () => void }) {
  return (
    <button
      onClick={onClick}
      className="w-full flex items-center gap-[12px] px-[16px] py-[12px] hover:bg-surface/50 transition-colors text-left"
    >
      <SymbolLogo symbol={info.symbol} size={36} />
      <div className="flex-1 min-w-0">
        <div className="text-[13px] font-medium text-txt truncate">{info.name}</div>
        <div className="text-[10px] text-txt-3 font-mono mt-[1px]">{info.symbol}</div>
      </div>
      <ChevronIcon />
    </button>
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

function ChevronIcon() {
  return (
    <svg viewBox="0 0 14 14" fill="none" stroke="currentColor" strokeWidth="1.5"
         className="w-[11px] h-[11px] text-txt-3 shrink-0">
      <polyline points="5,2 11,7 5,12"/>
    </svg>
  )
}
