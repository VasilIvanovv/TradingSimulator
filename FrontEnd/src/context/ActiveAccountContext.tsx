import {
  createContext, useContext, useState, useEffect, useCallback,
  type ReactNode,
} from 'react'
import { accountApi, type AccountInfo, type DepositAllResult } from '../api/trading'
import { setAccountId } from '../api/client'
import { useAuth } from './AuthContext'

interface ActiveAccountContextValue {
  accounts:      AccountInfo[]
  activeAccount: AccountInfo | null
  loading:       boolean
  setActive:     (account: AccountInfo) => void
  createAccount: (name: string) => Promise<AccountInfo | null>
  renameAccount: (id: number, name: string) => Promise<void>
  deleteAccount: (id: number) => Promise<void>
  deposit:       (id: number, amount: number) => Promise<void>
  depositAll:    (amount: number) => Promise<DepositAllResult>
  refresh:       () => Promise<void>
}

const ActiveAccountContext = createContext<ActiveAccountContextValue | null>(null)

export function ActiveAccountProvider({ children }: { children: ReactNode }) {
  const { isAuthenticated } = useAuth()
  const [accounts,      setAccounts]      = useState<AccountInfo[]>([])
  const [activeAccount, setActiveAccount] = useState<AccountInfo | null>(null)
  const [loading,       setLoading]       = useState(false)

  const loadAccounts = useCallback(async () => {
    setLoading(true)
    try {
      const list = await accountApi.list()
      setAccounts(list)
      // Always default to the first account
      if (list.length > 0) {
        setActiveAccount(list[0])
        setAccountId(list[0].id)
      } else {
        setActiveAccount(null)
        setAccountId(null)
      }
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    if (isAuthenticated) loadAccounts()
    else {
      setAccounts([])
      setActiveAccount(null)
      setAccountId(null)
    }
  }, [isAuthenticated, loadAccounts])

  function setActive(account: AccountInfo) {
    setActiveAccount(account)
    setAccountId(account.id)
  }

  async function createAccount(name: string): Promise<AccountInfo | null> {
    try {
      const account = await accountApi.create(name)
      setAccounts(prev => [...prev, account])
      return account
    } catch {
      return null
    }
  }

  async function renameAccount(id: number, name: string) {
    await accountApi.rename(id, name)
    setAccounts(prev => prev.map(a => a.id === id ? { ...a, name } : a))
    if (activeAccount?.id === id)
      setActiveAccount(prev => prev ? { ...prev, name } : prev)
  }

  async function deleteAccount(id: number) {
    await accountApi.delete(id)
    const next = accounts.filter(a => a.id !== id)
    setAccounts(next)
    if (activeAccount?.id === id) {
      const fallback = next[0] ?? null
      setActiveAccount(fallback)
      setAccountId(fallback?.id ?? null)
    }
  }

  async function deposit(id: number, amount: number) {
    await accountApi.deposit(id, amount)
  }

  async function depositAll(amount: number): Promise<DepositAllResult> {
    return accountApi.depositAll(amount)
  }

  // Don't render children until the first account load resolves — otherwise
  // any trading API call fires before accountId is set in client.ts and gets a 400.
  if (isAuthenticated && loading) return null

  return (
    <ActiveAccountContext.Provider value={{
      accounts, activeAccount, loading,
      setActive, createAccount, renameAccount, deleteAccount,
      deposit, depositAll, refresh: loadAccounts,
    }}>
      {children}
    </ActiveAccountContext.Provider>
  )
}

export function useActiveAccount() {
  const ctx = useContext(ActiveAccountContext)
  if (!ctx) throw new Error('useActiveAccount must be used inside ActiveAccountProvider')
  return ctx
}
