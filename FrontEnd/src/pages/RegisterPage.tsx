import { useState, FormEvent } from 'react'
import { useNavigate, Link } from 'react-router-dom'
import { useAuth } from '../context/AuthContext'
import { ApiError } from '../api/client'

export function RegisterPage() {
  const { register } = useAuth()
  const navigate = useNavigate()
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [loading, setLoading] = useState(false)

  async function handleSubmit(e: FormEvent) {
    e.preventDefault()
    setError(null)
    setLoading(true)
    try {
      await register(username, password)
      navigate('/', { replace: true })
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Registration failed')
    } finally {
      setLoading(false)
    }
  }

  return (
    <div className="min-h-screen bg-bg flex items-center justify-center">
      <div className="w-full max-w-sm">

        {/* Brand */}
        <div className="flex items-center gap-3 mb-10 justify-center">
          <div className="w-8 h-8 border border-accent bg-accent/10 rounded flex items-center justify-center font-mono text-[10px] font-bold text-accent">
            TS
          </div>
          <div>
            <div className="text-sm font-semibold text-txt leading-none">TradeSim</div>
            <div className="text-[10px] text-txt-3 uppercase tracking-widest">Simulator</div>
          </div>
        </div>

        {/* Card */}
        <div className="bg-card border border-border rounded p-7">
          <h1 className="text-base font-semibold text-txt mb-1">Create account</h1>
          <p className="text-[11px] text-txt-2 mb-6">Choose a username and password to get started</p>

          {error && (
            <div className="mb-4 px-3 py-2 rounded bg-neg/10 border border-neg/30 text-neg text-[11px]">
              {error}
            </div>
          )}

          <form onSubmit={handleSubmit} className="flex flex-col gap-4">
            <label className="flex flex-col gap-1">
              <span className="text-[10px] uppercase tracking-widest text-txt-3 font-semibold">Username</span>
              <input
                type="text"
                autoComplete="username"
                value={username}
                onChange={e => setUsername(e.target.value)}
                required
                className="bg-surface border border-border rounded px-3 py-2 text-txt text-[12px] outline-none
                           focus:border-accent transition-colors placeholder:text-txt-3"
                placeholder="your_username"
              />
            </label>

            <label className="flex flex-col gap-1">
              <span className="text-[10px] uppercase tracking-widest text-txt-3 font-semibold">Password</span>
              <input
                type="password"
                autoComplete="new-password"
                value={password}
                onChange={e => setPassword(e.target.value)}
                required
                className="bg-surface border border-border rounded px-3 py-2 text-txt text-[12px] outline-none
                           focus:border-accent transition-colors placeholder:text-txt-3"
                placeholder="••••••••"
              />
            </label>

            <button
              type="submit"
              disabled={loading}
              className="mt-1 py-2 rounded bg-accent text-[#050C18] text-[12px] font-semibold
                         hover:brightness-110 transition-all disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {loading ? 'Creating account…' : 'Create account'}
            </button>
          </form>
        </div>

        <p className="text-center text-[11px] text-txt-3 mt-5">
          Already have an account?{' '}
          <Link to="/login" className="text-accent hover:underline">Sign in</Link>
        </p>
      </div>
    </div>
  )
}
