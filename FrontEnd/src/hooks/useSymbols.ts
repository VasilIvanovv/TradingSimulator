import { useState, useEffect } from 'react'
import { tradingApi, type SymbolInfo } from '../api/trading'

// Module-level cache — fetched once per browser session, shared across all
// components that call useSymbols(). Avoids redundant network requests when
// the picker is opened and closed multiple times.
let cache: SymbolInfo[] | null = null

interface UseSymbolsResult {
  symbols: SymbolInfo[]
  loading: boolean
}

export function useSymbols(): UseSymbolsResult {
  const [symbols, setSymbols] = useState<SymbolInfo[]>(cache ?? [])
  const [loading, setLoading] = useState(cache === null)

  useEffect(() => {
    if (cache !== null) return
    tradingApi.getSymbols()
      .then(data => { cache = data; setSymbols(data) })
      .catch(() => { /* keep empty list, picker still usable */ })
      .finally(() => setLoading(false))
  }, [])

  return { symbols, loading }
}
