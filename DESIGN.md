# Agent-Based DAW UI Design System
> Adapted from CollarAgent web taste to the DAW dark theme defined in `src/Presentation/views/theme.cpp` / `theme.h`. Token names keep web-style naming (`--color-surface-*`, `--color-primary`, etc.) but values are now the Deep Cyber-Industrial dark palette.

## Overview

Agent-Based DAW (Strata Studio) is a desktop Qt6 application with a modern, minimalist design featuring a top transport bar + vertical splitter (Playlist / PianoRoll) + floating Mixer + left Browser. The design system uses a warm-dark neutral scale with soft-violet primary accent, built on centralized QSS in `theme.cpp` with Inter font.

### Design Philosophy

- **Dark & Minimalist**: Clean, spacious interfaces with warm-dark backgrounds (`#161616` → `#323232`)
- **Functional Over Decorative**: Every element serves a clear purpose
- **Responsive**: Fluid layouts that adapt to user preferences (splitters, resizable panels)
- **Accessible**: High contrast warm-white text (`#E8E8E8`) on dark surfaces with clear visual hierarchy

---

## Color System
> Source of truth: `presentation::theme::Color` in `src/Presentation/views/theme.h`. Do not introduce hard-coded colors in `views/` — add a token here + in `theme.h/.cpp` first.

### Primary Palette — Surfaces

| Token | Value | Qt Equivalent | Usage |
| ----- | ----- | ------------- | ----- |
| `--color-surface-50` | `#161616` | `Color::BgBase` | App shell, main window, deepest background |
| `--color-surface-100` | `#222222` | `Color::BgSurface` | Panels, tracks, cards, `mixerPanel`, `timelinePanel` |
| `--color-surface-200` | `#323232` | `Color::BgControl` | Controls, inactive buttons, inputs, borders |
| `--color-surface-300` | `#3A4454` | `Color::BgSlotActive` | Elevated / hover surface, active slots |
| `--color-slot-empty` | `#1C1E24` | `Color::BgSlotEmpty` | Empty slot background |
| `--color-slot-bypassed` | `#2C323F` | `Color::BgSlotBypassed` | Bypassed slot background |

### Borders

| Token | Value | Qt Equivalent | Usage |
| ----- | ----- | ------------- | ----- |
| `--color-border-subtle` | `#323232` | `BgControl` | Default 1px separation, card borders |
| `--color-border-slot-empty` | `#2B303C` | `BorderSlotEmpty` | Empty slot border |
| `--color-border-slot-bypassed` | `#3F4758` | `BorderSlotBypassed` | Bypassed slot border |
| `--color-border-slot-active` | `#505E78` | `BorderSlotActive` | Active slot border |
| `--color-grid-overlay` | `rgba(255,255,255,0.02)` | `GridOverlay` | Timeline / canvas grid overlay |

### Accent Colors

| Token | Value | Qt Equivalent | Usage |
| ----- | ----- | ------------- | ----- |
| `--color-primary` | `#A78BFA` | `Color::AccentGlow` | Primary actions, active states, focus rings, checked buttons |
| `--color-primary-hover` | `#C4B5FD` | — (lightened Glow) | Hover on primary / default dialog button |
| `--color-accent` | `#3B82F6` | `Color::AccentAudio` | Secondary highlights, links, audio tracks, monitor |
| `--color-accent-midi` | `#A855F7` | `Color::AccentMIDI` | MIDI tracks, synth events |
| `--color-accent-glow` | `rgba(167,139,250,0.4)` | — | Glow effects, `drawVolumetricGlow` peak alpha |
| `--color-danger` | `#FF3B30` | `Color::AccentRecord` | Record-arm, preview-playing, destructive |
| `--color-danger-border` | `#FF5555` | `Color::BorderAlert` | Alert border |
| `--color-danger-bg` | `#1A0D0D` | `Color::BgAlertMuted` | Muted alert background |
| `--color-warning` | `#D4A84B` | `Color::BtnMuteActive` | Mute active (amber) |
| `--color-success` | `#2DA87E` | `Color::BtnSoloActive` | Solo active (jade) |
| `--color-plugin` | `#E65C00` | `Color::PluginActive` | Active plugin LED / instrument+effect selected menu |
| `--color-plugin-muted` | `#7E8A9F` | `Color::PluginBypassed` | Bypassed plugin |
| `--color-send-pre` | `#00D2B4` | `Color::SendPreFader` | Pre-fader send active (teal) |
| `--color-send-post` | `#5096FF` | `Color::SendPostFader` | Post-fader send active (bright blue) |
| `--color-safety-amber` | `#FFB300` | `Color::SafetyAmber` | Unity / telemetry warning line |

### Text Colors

| Token | Value | Qt Equivalent | Usage |
| ----- | ----- | ------------- | ----- |
| `--ev-c-text-1` | `#E8E8E8` | `Color::TextPrimary` | Primary text, headings |
| `--ev-c-text-2` | `#9DB2BF` | `Color::TextSecondary` | Secondary text, body content |
| `--ev-c-text-3` | `#888888` | `Color::TextMuted` | Tertiary text, placeholders, disabled, grids |
| `--ev-c-text-light` | `#DDE6ED` | `Color::TextLight` | Soft light white, popup values |
| `--ev-c-text-disabled` | `#565C66` | — (QSS disabled) | Disabled buttons / menu items |
| `--ev-c-text-on-dark` | `#161616` | `BtnMuteText` / `BtnSoloText` | Text on amber / jade chips |

### Semantic Mappings

```css
--color-background: var(--color-surface-50); /* #161616 BgBase */
--color-background-soft: var(--color-surface-100); /* #222222 BgSurface */
--color-background-mute: var(--color-surface-200); /* #323232 BgControl */
--color-text: var(--ev-c-text-1);
--color-border: var(--color-border-subtle);
```

```cpp
// Qt mapping — use these, never raw hex in views/
Color::BgBase      // #161616
Color::BgSurface   // #222222
Color::BgControl   // #323232
Color::AccentGlow  // #A78BFA
Color::TextPrimary // #E8E8E8
Color::TextMuted   // #888888
```

### Color Usage Guidelines

1. **Backgrounds**: Use `surface-50` for shell, `surface-100` for panels/cards, `surface-200` for inputs/buttons.
2. **Borders**: Always use `border-subtle` (`#323232`) or slot borders for separation. Never pure black. Current outlier `#526D82` dividers in `main_window.cpp` / `top_control_panel.cpp` should migrate to `BgControl`.
3. **Primary Actions**: Use `primary` (`#A78BFA`) for checked states, focus rings (`1px solid AccentGlow`), default dialog buttons. Hover → `#C4B5FD`.
4. **Highlights**: Use `accent` (`#3B82F6`) for audio/links/monitor, `accent-midi` (`#A855F7`) for MIDI, `danger` (`#FF3B30`) for record.
5. **Hover States**: `surface-200 → surface-300` background + white text (`#FFFFFF`) for buttons; `surface-100 → surface-200` for list items. Card hover adds `primary` border + soft glow shadow.
6. **Track identity**: Audio Clips cobalt `#3B82F6`, MIDI amethyst `#A855F7`, Automation Clips violet `#A78BFA` — do not swap.

---

## Typography

### Font Stack

```css
font-family:
  Inter,
  -apple-system,
  BlinkMacSystemFont,
  'Segoe UI',
  Roboto,
  Oxygen,
  Ubuntu,
  Cantarell,
  'Fira Sans',
  'Droid Sans',
  'Helvetica Neue',
  sans-serif;
```

> Qt: `Font::primary()` / `Font::monospace()` (both Inter today, tabular figures planned). `Font::initialize()` loads `src/assets/fonts/Inter/*`. QSS uses `'Inter'` 11–14px + `scaleFontSize()` boost.

### Text Scale

| Class | Size | Qt (`Font::Size*`) | Usage |
| ----- | ---- | ------------------ | ----- |
| `text-4xl` | 36px | `SizeTitle 22 +4` | Page titles, hero headings |
| `text-2xl` | 24px | — | Section titles, modal headers |
| `text-xl` | 20px | `SizeHeader 14 →18` | Card titles, panel headings |
| `text-lg` | 18px | — | Subheadings, important labels |
| `text-base` | 16px | `SizePrimary 11 →14` | Body text, default size |
| `text-sm` | 14px | `SizeSecondary 10 →13` | Secondary text, labels, QSS 13px base |
| `text-xs` | 12px | `SizeDetail 9 →12` | Meta information, timestamps |

### Font Weights

| Class | Weight | Usage |
| ----- | ------ | ----- |
| `font-extrabold` | 800 | Emphasized headings |
| `font-bold` | 700 | Primary headings, M/S/R/I chips, header-btns |
| `font-semibold` | 600 | Subheadings, emphasized text, dock titles |
| `font-medium` | 500 | Labels, important content, QSS buttons/inputs |
| `font-normal` | 400 | Body text, default |

### Special Typography

- **Monospace**: `font-mono` for code, time display, file paths, BPM (`QLineEdit#bpmSelector` bold 14px violet)
- **Tracking**: Use `tracking-tight` for headings, `tracking-wide` for labels
- **Line Height**: Base `1.6` for body text

---

## Spacing & Layout

### Border Radius

| Token | Value | Qt today | Usage |
| ----- | ----- | -------- | ----- |
| `--radius-sm` | 4px | QSS chips, inputs-in-dialog, M/S/R/I | Small elements, badges |
| `--radius-md` | 8px | **target** (Qt today 6px) | Buttons, inputs |
| `--radius-lg` | 12px | **target** (Qt today 6px) | Cards, panels |
| `--radius-xl` | 16px | **target** | Large cards, modals |

> Web taste uses 8/12/16. Qt `theme.cpp` currently uses 6px everywhere + 4px chips. When modernizing `views/`, step up to 8/12 without breaking density.

### Spacing Scale

- **Compact**: `p-1`, `p-2`, `px-1.5`, `py-0.5` - Dense UI, icons
- **Standard**: `p-3`, `px-3 py-2`, `p-4` - Default spacing
- **Generous**: `p-6`, `px-4 sm:px-6` - Modal content, sections
- **Gap**: `gap-1`, `gap-2`, `gap-3`, `gap-4` - Element spacing

Qt translation: `contentsMargins(12,12,12,12)`, `spacing 8`, QSS `padding: 6px 12px / 8px 12px`. Avoid `spacing 0` full-bleed (see `main_window.cpp` rootLayout today).

### Layout System

#### Flexbox Patterns

**Vertical Stack:**

```tsx
<div className="flex flex-col h-full">
  <div className="shrink-0">Header</div>
  <div className="flex-1 overflow-hidden">Content</div>
</div>
```

**Horizontal Toolbar:**

```tsx
<div className="flex items-center justify-between p-3">
  <div className="flex items-center gap-2">Left actions</div>
  <div className="flex items-center gap-1">Right actions</div>
</div>
```

**Expandable Content:**

```tsx
<div className="flex flex-col">
  <div className="flex items-center justify-between cursor-pointer">Header</div>
  {isExpanded && <div className="animate-in fade-in">Content</div>}
</div>
```

#### Grid Patterns

**Responsive Cards:**

```tsx
<div className="grid grid-cols-1 md:grid-cols-2 gap-6">{/* Cards */}</div>
```

### App Layout Structure

```
┌─────────────────────────────────────────────────────────┐
│ TopControlPanel (h-48px): InputMode | Transport | WsCtl │
├─────────────┬───────────────────────────┬───────────────┤
│ Browser     │ Vertical Splitter         │ Mixer         │
│ (left)      │ Playlist (top, ~740)      │ (floating    │
│             │ PianoRoll (bottom,hidden) │  window)      │
│ Tree+Search │ TimelineRuler + Clips     │ Strips 140px  │
│ PreviewDeck │ TrackHeaders + Canvas     │ Master 170px  │
└─────────────┴───────────────────────────┴───────────────┘
```

**Width Constraints:**

- Browser: resizable, ~240px default
- Mixer strips: 140px (`Layout::TrackStripWidth`), Master 170px
- Track heights: 24–256px, default 72px (`Layout::DefaultTrackHeight`)
- Sub-lane (Automation Clip lane): 24–200px, default 60px
- Reading Column (`reading-column`): 100% up to `48rem` (768px) for dialogs / settings

---

## Component Architecture

### Component Organization

```
src/Presentation/views/
├── theme.{h,cpp}      - Centralized QSS + Color/Font/Layout/PaintHelper
├── main_window.{h,cpp}- Shell: TopControlPanel + Splitter + Browser + Playlist
├── top_control_panel/ - InputMode / Transport / Workspace + TimeDisplay
├── playlist/          - PlaylistWindow, TrackHeaderView, ClipCanvas, dialogs
├── pianoroll_window/  - PianoRollWindow, Canvas, Velocity/Controller lanes
├── mixer/             - MixerWindow, ChannelStrip, slots, fader/dial/meter
├── browser_window/    - BrowserWidget, Tree/Search/Navigation/PreviewDeck
├── settings/          - SettingsDialog
└── boot/              - SplashScreen
```

### Naming Conventions

| Pattern | Examples | Usage |
| ------- | -------- | ----- |
| PascalCase | `MessageList`, `ToolCallCard` | All React components (web ref) |
| PascalCase + Widget/Window | `ChannelStripWidget`, `PlaylistWindow` | All Qt widgets |
| `Card` suffix | `GenericToolCard`, `WorkspaceCard` | Display cards (web) |
| `Modal`/`Dialog` suffix | `CreateSkillModal`, `RenderSettingsDialog` | Modal dialogs |
| `Pane`/`Panel` suffix | `SubagentStreamPane`, `PickerPanel` | Dedicated panels |
| `Container` suffix | `ChatContainer`, `EffectSlotContainer` | Wrapping containers |

---

## Design Patterns (Dark-Theme Versions)

### Card Pattern

```tsx
<div className="bg-surface-100/80 border border-surface-200 rounded-xl overflow-hidden hover:shadow-[0_0_24px_var(--color-accent-glow)] hover:border-primary transition-all">
  <div className="flex items-center justify-between p-3 cursor-pointer">
    <div className="flex items-center gap-3">
      {/* Icon in text-primary */}
      <span className="font-medium text-[var(--ev-c-text-1)]">Title</span>
    </div>
    <div>{ExpandIcon}</div>
  </div>
  {isExpanded && <div className="px-3 pb-3 animate-in fade-in">{/* Content */}</div>}
</div>
```

Qt equivalent: `QFrame` / `QWidget#mixerPanel,#timelinePanel` with `background: #222222; border-radius: 12px; border: 1px solid #323232;` + hover → border `#A78BFA`. Replaces flat 6px panels in `getCoreStyleSheet()`.

**Variants:**

- Generic card: Neutral `#222222` styling
- Tool card: Specialized icons and content
- Subagent card: Violet theme (`#A78BFA` border + glow) with status badges
- Action card: Larger with hover glow

### Modal Pattern

```tsx
<div className="fixed inset-0 z-50 bg-black/60 backdrop-blur-sm flex items-center justify-center p-4">
  <div className="bg-surface-100 w-full max-w-2xl rounded-xl shadow-2xl border border-surface-200 overflow-hidden flex flex-col">
    <div className="flex items-center justify-between p-4 border-b border-surface-200">
      <h3 className="text-xl font-semibold text-[var(--ev-c-text-1)]">Title</h3>
      <button onClick={onClose} className="p-1 hover:bg-surface-200 rounded">
        <CloseIcon />
      </button>
    </div>
    <div className="flex-1 overflow-y-auto">{Content}</div>
  </div>
</div>
```

Qt: `QDialog#DAWInputDialog { background: #161616; border: 1px solid #323232; border-radius: 12px; }` — already close, just bump radius 6→12.

**Sizes:**

- Small: `max-w-sm`
- Medium: `max-w-2xl` (default)
- Large: `max-w-4xl`
- Full screen: `w-full h-full`

### Button Patterns

**Primary Button (violet):**

```tsx
<button className="px-4 py-2 bg-primary hover:bg-[#C4B5FD] text-[#161616] rounded-lg transition-colors">
  Action
</button>
```

Qt: `QPushButton:checked / :default { background: #A78BFA; color: #161616; border-radius: 8px; padding: 6px 16px; }`

**Secondary Button:**

```tsx
<button className="px-4 py-2 bg-surface-200 hover:bg-surface-300 text-[var(--ev-c-text-1)] rounded-lg transition-colors">
  Cancel
</button>
```

**Danger Button (record/remove):**

```tsx
<button className="px-4 py-2 bg-[#59161E] hover:bg-[#FF3B30] hover:text-white text-[#FF6B6B] rounded-lg transition-colors">
  Remove
</button>
```

**Icon Button:**

```tsx
<button className="p-1.5 hover:bg-surface-200 text-[var(--ev-c-text-3)] hover:text-[var(--ev-c-text-1)] rounded-md transition-colors">
  <Icon width={16} height={16} />
</button>
```

Qt: `QPushButton#infoBtn,#collapseBtn { background: transparent; color: #9DB2BF; } :hover { color: #E8E8E8; }`

### Input Pattern

```tsx
<input
  type="text"
  className="w-full p-3 border border-surface-200 rounded-lg bg-surface-50 text-[var(--ev-c-text-1)] placeholder:text-[var(--ev-c-text-3)] focus:outline-none focus:ring-2 focus:ring-primary/50 focus:border-primary transition-all"
/>
```

Qt: `QLineEdit, QSpinBox, QDoubleSpinBox { background: #161616; border-radius: 8px; padding: 6px 12px; color: #E8E8E8; } :focus { border: 1px solid #A78BFA; }` — add explicit border to match web focus ring.

### Toolbar Pattern

```tsx
<div className="flex items-center justify-between p-3 border-b border-surface-200 bg-surface-50/80 backdrop-blur-sm">
  <div className="flex items-center gap-2">
    <h2 className="font-semibold text-[var(--ev-c-text-1)]">Title</h2>
  </div>
  <div className="flex items-center gap-1">
    {actions.map((action) => (
      <button key={action} className="p-1.5 hover:bg-surface-200 rounded-md">
        {icon}
      </button>
    ))}
  </div>
</div>
```

Qt target for `TopControlPanel` (48px, glass `BgBase` + bottom `BgControl` border today): add `12px` horizontal padding, `8px` gaps, pill transport group.

---

## Interactive States (Dark)

### Hover States

**Button Hover:**

```tsx
className = 'hover:bg-surface-300 hover:text-white transition-colors'
```

**Card Hover:**

```tsx
className =
  'hover:shadow-lg hover:border-primary hover:shadow-[var(--color-accent-glow)] transition-all duration-300'
```

**Icon Hover:**

```tsx
className = 'group-hover:scale-110 transition-transform duration-300'
```

### Active/Selected States

```tsx
className={isSelected
  ? 'bg-surface-200 font-medium border-l-2 border-primary text-[var(--ev-c-text-1)]'
  : 'hover:bg-surface-100 text-[var(--ev-c-text-3)] border-l-2 border-transparent'
}
```

Qt: `QTreeView::item:selected { background: #323232; color: #A78BFA; } :hover { background: #222222; }`

### Disabled States

```tsx
disabled = { isDisabled }
className = 'disabled:opacity-50 disabled:cursor-not-allowed transition-all'
```

Qt: `color: #565C66; background: transparent;`

### Loading States

```tsx
{
  isLoading && (
    <div className="absolute inset-0 bg-surface-50/60 backdrop-blur-sm flex items-center justify-center">
      <LoadingIcon className="animate-spin text-primary" />
    </div>
  )
}
```

---

## Animations

### Animation Utilities

```tsx
// Fade in with zoom
className = 'animate-in fade-in zoom-in-95 duration-200'

// Slide from top
className = 'animate-in fade-in slide-in-from-top-4 duration-500'

// Slide from right
className = 'animate-in fade-in slide-in-from-right-4 duration-300'

// With delay
className = 'animate-in fade-in zoom-in-95 duration-500 delay-150'
```

> Qt has no CSS animation; translate to `QPropertyAnimation` 150–300ms (fade/slide for expand, modal zoom). Keep durations identical for parity.

### Common Animations

| Use Case | Classes / Qt |
| -------- | ------------ |
| Modal open | `fade-in zoom-in-95 duration-200` |
| Panel expand | `fade-in slide-in-from-top-2 duration-200` |
| Welcome card | `fade-in slide-in-from-top-4 duration-500 delay-150` |
| Content load | `fade-in duration-300` |

### Transitions

```tsx
// Smooth all properties
className = 'transition-all duration-200'

// Color transitions only
className = 'transition-colors duration-200'

// Transform transitions only
className = 'transition-transform duration-300'
```

---

## Third-Party Integrations

### Dockview (web ref) → QSplitter / QDockWidget (Qt)

**Custom Theme (dark):**

```css
.dockview-theme-custom {
  --dv-background-color: var(--color-surface-50);
  --dv-tabs-and-actions-container-background-color: var(--color-surface-100);
  --dv-activegroup-visiblepanel-tab-background-color: var(--color-surface-50);
  --dv-active-sash-color: var(--color-primary);
  --dv-font-family: inherit;
}
```

**Qt Usage:**

- `QSplitter::handle { background: transparent; }` hover → `primary`
- `QDockWidget::title { background: #161616; color: #888888; font-size: 13px; font-weight: 600; }`
- Remove hard-coded `#526D82` splitter/divider inline styles — use `BgControl` → hover `AccentGlow`.

---

## Best Practices

### Do's

1. **Use surface colors consistently**: `#161616` shell, `#222222` panels, `#323232` controls/inputs.
2. **Provide clear visual feedback**: Hover glow, `primary` focus rings, loading overlays.
3. **Maintain proper spacing**: `p-3/p-4/gap-2/gap-3` → Qt `12px margins / 8px spacing`. No zero-gap full-bleed except canvas.
4. **Use rounded corners consistently**: `8px` inputs/buttons, `12px` cards/panels/dialogs, `4px` chips/badges.
5. **Support text truncation**: Use `truncate` / `elide` and `max-w-*` for track names, plugin names.
6. **Handle overflow properly**: Use `overflow-hidden` containers with inner scrollable areas (`QScrollArea` borderless + 8px custom scrollbars).
7. **Use semantic colors**: Text hierarchy with `ev-c-text-1/2/3` → `TextPrimary/Secondary/Muted`. Audio Clip cobalt, MIDI amethyst, Automation Clip violet.

### Don'ts

1. **Don't use hard-coded colors**: Always use design tokens / `Color::*`. Grep for `526D82`, `565C66`, `#424242` left in QSS and migrate.
2. **Don't ignore hover states**: Interactive elements need glow/border feedback.
3. **Don't use arbitrary spacing**: Follow the spacing scale.
4. **Don't forget accessibility**: `#E8E8E8` on `#222222` ≈ 13:1; `#888888` on `#222222` is decorative only — never body text. Ensure keyboard focus rings.
5. **Don't mix patterns**: Follow established component patterns (header-btn `accent/warning/danger` variants).

### Performance

1. **Use Tailwind JIT**: Tailwind v4 compiles only used classes (web ref)
2. **Minimize custom CSS**: Prefer utilities; in Qt prefer centralized `theme.cpp` QSS over per-widget inline `setStyleSheet`
3. **Use `shrink-0` for fixed elements**: Prevent flex children from shrinking; Qt: proper `SizePolicy::Fixed` + stretch factors
4. **Use `overflow-hidden` carefully**: Prevent unnecessary layout recalculations; Qt: `WaveformTileCache` + canvas clipping
5. **Prefer `PaintHelper`**: `drawGlassPanel`, `drawVolumetricGlow`, `drawControlGrip` over extra widget layers

### Accessibility

1. **Color contrast**: `#E8E8E8` on `#161616`/`#222222` meets WCAG AAA. `#A78BFA` on `#161616` meets AA for large text/UI — use dark text (`#161616`) on violet fills for small text.
2. **Keyboard navigation**: All interactive elements should be keyboard-accessible
3. **Focus states**: Use `focus:ring-2 focus:ring-primary/50` → Qt `border: 1px solid #A78BFA`
4. **ARIA labels**: Add appropriate labels for icons and buttons without text
5. **Semantic HTML**: Use proper elements (button, input, nav, etc.)

---

## Icon System

### Usage Pattern

```tsx
import { PlusIcon } from './assets/icons'

;<PlusIcon width={16} height={16} className="text-current" />
```

Qt: `PaintHelper::createSvgIcon(path, QSize(20,20))` — Normal `#888888`, Active `#E8E8E8`, Selected `#A78BFA`, Disabled `#565C66`.

### Icon Sizes

| Width/Height | Usage |
| ------------ | ----- |
| 12px - 14px | Small icons, dense UI (M/S/R/I chips) |
| 16px - 18px | Default size, buttons |
| 20px - 24px | Larger icons, headings, transport |
| 32px+ | Hero icons, illustrations, empty states |

### Icon Colors

- Default: `text-current` → `#888888` muted
- Primary: `text-primary` → `#A78BFA` violet
- Accent: `text-accent` → `#3B82F6` cobalt
- Accent MIDI: `#A855F7` amethyst
- Secondary: `text-[var(--ev-c-text-2)]` → `#9DB2BF`
- Muted: `text-[var(--ev-c-text-3)]` → `#888888`
- Danger: `#FF3B30` record / `#FF6B6B` remove

---

## Code Examples

### Complete Card Component (web ref → Qt target)

```tsx
interface CardProps {
  icon: React.ReactNode
  title: string
  content?: React.ReactNode
  defaultExpanded?: boolean
}

export function Card({ icon, title, content, defaultExpanded = false }: CardProps) {
  const [isExpanded, setIsExpanded] = useState(defaultExpanded)

  return (
    <div className="bg-surface-100/80 border border-surface-200 rounded-xl overflow-hidden hover:shadow-md hover:border-primary transition-all">
      <div
        className="flex items-center justify-between p-3 cursor-pointer hover:bg-surface-200/50"
        onClick={() => setIsExpanded(!isExpanded)}
      >
        <div className="flex items-center gap-3">
          <div className="text-primary">{icon}</div>
          <span className="font-medium text-[var(--ev-c-text-1)]">{title}</span>
        </div>
        <ChevronDownIcon
          width={16}
          height={16}
          className={`transition-transform duration-200 ${isExpanded ? 'rotate-180' : ''}`}
        />
      </div>
      {isExpanded && (
        <div className="px-3 pb-3 animate-in fade-in slide-in-from-top-2 duration-200">
          {content}
        </div>
      )}
    </div>
  )
}
```

### Complete Modal Component

```tsx
interface ModalProps {
  isOpen: boolean
  onClose: () => void
  title: string
  children: React.ReactNode
  size?: 'sm' | 'md' | 'lg' | 'full'
}

export function Modal({ isOpen, onClose, title, children, size = 'md' }: ModalProps) {
  if (!isOpen) return null

  const sizeClasses = {
    sm: 'max-w-sm',
    md: 'max-w-2xl',
    lg: 'max-w-4xl',
    full: 'w-full h-full'
  }

  return (
    <div className="fixed inset-0 z-50 bg-black/60 backdrop-blur-sm flex items-center justify-center p-4 animate-in fade-in duration-200">
      <div
        className={`bg-surface-100 border border-surface-200 ${sizeClasses[size]} rounded-xl shadow-2xl overflow-hidden flex flex-col animate-in zoom-in-95 duration-200`}
      >
        <div className="flex items-center justify-between p-4 border-b border-surface-200 shrink-0">
          <h3 className="text-xl font-semibold text-[var(--ev-c-text-1)]">{title}</h3>
          <button
            onClick={onClose}
            className="p-1 hover:bg-surface-200 text-[var(--ev-c-text-3)] hover:text-[var(--ev-c-text-1)] rounded-md transition-colors"
            aria-label="Close"
          >
            <CloseIcon width={20} height={20} />
          </button>
        </div>
        <div className="flex-1 overflow-y-auto">{children}</div>
      </div>
    </div>
  )
}
```

---

## Resources

### File Structure

- `DESIGN.md` - This spec (web taste mapped to DAW dark tokens)
- `src/Presentation/views/theme.h` - `Color` / `Font` / `Layout` / `Style` / `PaintHelper` declarations
- `src/Presentation/views/theme.cpp` - Centralized QSS + painting (single source of truth for values)
- `src/assets/fonts/Inter/*` - Inter variable fonts
- Web ref: `src/renderer/assets/base.css`, `src/renderer/App.tsx`, `src/renderer/components/`

### Key Tools (Qt mapping)

- **Qt6 QSS + QPainter**: Replaces Tailwind utilities (`theme.cpp` centralizes)
- **QSplitter / QDockWidget**: Replaces Dockview tabbed workspace
- **Inter**: Shared font family
- **Presentation Director (60Hz)**: Telemetry / meter updates

### Related Documentation

- Tailwind CSS: https://tailwindcss.com
- Dockview: https://dockview.dev
- React: https://react.dev
- Qt6 QSS: https://doc.qt.io/qt-6/stylesheet-reference.html
