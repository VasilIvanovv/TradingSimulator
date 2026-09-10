import { api, withAccount } from './client'

// ── Types matching the backend JSON exactly ───────────────────────────────────

export interface SymbolInfo {
  symbol: string
  name:   string
  sector: string
}

export interface TradeRecord {
  symbol:         string
  side:           'buy' | 'sell'
  quantity:       number
  price:          number
  timestamp:      string
  executionPrice: number
  status:         'filled' | 'rejected'
}

export interface AccountSnapshot {
  cash:         number
  positions:    Record<string, number>
  tradeHistory: TradeRecord[]
}

export interface ExecutionReceipt {
  status:         'filled' | 'rejected'
  executionPrice: number
}

export interface ActiveRule {
  symbol:       string
  triggerPrice: number
  side:         'buy' | 'sell'
  quantity:     number
}

export interface PriceCandle {
  timestamp: string
  open:      number
  high:      number
  low:       number
  close:     number
  volume:    number
}

export interface AccountInfo {
  id:   number
  name: string
}

export interface DepositAllResult {
  succeeded: number[]
  failed:    number[]
}

// ── Account management ────────────────────────────────────────────────────────

export const accountApi = {
  list: () =>
    api.get<AccountInfo[]>('/accounts'),

  create: (name: string) =>
    api.post<AccountInfo>('/accounts', { name }),

  rename: (id: number, name: string) =>
    api.put<void>(`/accounts/${id}`, { name }),

  delete: (id: number) =>
    api.delete<void>(`/accounts/${id}`),

  deposit: (id: number, amount: number) =>
    api.post<void>(`/accounts/${id}/deposit`, { amount }),

  depositAll: (amount: number) =>
    api.post<DepositAllResult>('/accounts/deposit-all', { amount }),
}

// ── Trading (all account-scoped via ?accountId=) ──────────────────────────────

export const tradingApi = {
  getAccount: () =>
    api.get<AccountSnapshot>(withAccount('/account')),

  placeOrder: (symbol: string, side: 'buy' | 'sell', quantity: number, price: number) =>
    api.post<ExecutionReceipt>(withAccount('/orders'), {
      symbol, side, quantity, price,
      timestamp: new Date().toISOString().slice(0, 19).replace('T', ' '),
    }),

  getRules: () =>
    api.get<ActiveRule[]>(withAccount('/rules')),

  addRule: (symbol: string, triggerPrice: number, side: 'buy' | 'sell', quantity: number) =>
    api.post<void>(withAccount('/rules'), { symbol, triggerPrice, side, quantity }),

  removeRules: (symbol: string) =>
    api.delete<void>(withAccount(`/rules/${encodeURIComponent(symbol)}`)),

  getSymbols: () =>
    api.get<SymbolInfo[]>('/symbols'),

  getHistory: (symbol: string, interval: string, start: string) =>
    api.get<{ candles: PriceCandle[] }>(
      withAccount(`/history?symbol=${encodeURIComponent(symbol)}&interval=${encodeURIComponent(interval)}&start=${encodeURIComponent(start)}`)
    ),
}
