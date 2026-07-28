/** @type {import('tailwindcss').Config} */
export default {
  content: ['./index.html', './src/**/*.{ts,tsx}'],
  theme: {
    extend: {
      colors: {
        bg:           '#070D1A',
        surface:      '#0C1526',
        card:         '#101C34',
        border:       '#1A2D4A',
        'border-dim': '#0F1D31',
        txt:          '#C9D8EC',
        'txt-2':      '#5D7898',
        'txt-3':      '#3A5270',
        accent:       '#00CFAB',
        pos:          '#00CFAB',
        neg:          '#FF4158',
        link:         '#4D9FFF',
      },
      fontFamily: {
        mono: ['"Courier New"', 'Menlo', 'monospace'],
      },
    },
  },
  plugins: [],
}
