import { useState, useEffect, useCallback } from 'react'
import { tradingApi, type AccountSnapshot } from '../api/trading'

interface UseAccountResult {
  account:  AccountSnapshot | null
  loading:  boolean
  error:    string | null
  refresh:  () => void
}

export function useAccount(): UseAccountResult {
  const [account, setAccount] = useState<AccountSnapshot | null>(null)
  const [loading, setLoading] = useState(true)
  const [error, setError]     = useState<string | null>(null)

  const fetch = useCallback(async () => {
    setLoading(true)
    setError(null)
    try {
      setAccount(await tradingApi.getAccount())
    } catch (e) {
      setError(e instanceof Error ? e.message : 'Failed to load account')
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => { fetch() }, [fetch])

  return { account, loading, error, refresh: fetch }
}
