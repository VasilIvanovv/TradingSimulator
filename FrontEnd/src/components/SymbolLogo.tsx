import { useState } from 'react'
import { BASE } from '../api/client'

const AVATAR_COLORS = [
  '#1A3A5C', '#1A4A3A', '#3A1A4A', '#4A2A1A',
  '#1A3A4A', '#3A3A1A', '#1A1A4A', '#3A1A1A',
]

interface Props {
  symbol: string
  size:   number
}

export function SymbolLogo({ symbol, size }: Props) {
  const [failed, setFailed] = useState(false)
  const style = { width: size, height: size, minWidth: size }

  if (failed) {
    const bg = AVATAR_COLORS[symbol.charCodeAt(0) % AVATAR_COLORS.length]
    return (
      <div
        style={{ ...style, background: bg, fontSize: Math.round(size * 0.38) }}
        className="rounded-full flex items-center justify-center font-mono font-bold text-white"
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
