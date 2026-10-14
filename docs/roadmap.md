# Roadmap — Visual Plan

This page is the visual plan for the first three milestones (M1 boots → M2 plays → M3 LAN with friends).
Each box is a **session**: a unit of work that fits in one sitting and ships as one PR.
Details for each session are in [`sessions/`](sessions/README.md).

## Phases at a glance

```mermaid
flowchart LR
    P0["Phase 0<br/>Foundation<br/>(wiki & plan)"]:::done --> P1["Phase 1<br/>Understand & strip<br/>S01–S05"]
    P1 --> P2["Phase 2<br/>Make it build<br/>S06–S09"]
    P2 --> M1(["M1<br/>It boots"]):::milestone
    M1 --> P3["Phase 3<br/>Make it play<br/>S10–S11"]
    P3 --> M2(["M2<br/>It plays"]):::milestone
    M2 --> P4["Phase 4<br/>LAN multiplayer<br/>S12–S15"]
    P4 --> M3(["M3<br/>LAN with friends"]):::milestone
    M3 -.-> Later["Later: internet play,<br/>lobby server, matchmaking, ranked"]:::later

    classDef done fill:#2e7d32,color:#fff,stroke:#1b5e20
    classDef milestone fill:#f9a825,color:#000,stroke:#f57f17
    classDef later fill:#eeeeee,color:#555,stroke:#999,stroke-dasharray: 4 3
```

## Session dependency graph

An arrow means "must finish before". Sessions in the same **wave** don't depend on each other,
so separate agents can work on them in parallel.

```mermaid
flowchart TB
    subgraph W1["Wave 1 — parallel, mostly read-only"]
        S01["S01<br/>Audit: game code"]
        S02["S02<br/>Audit: support libraries<br/>& assembly"]
        S03["S03<br/>CMake + CI skeleton"]
        S04["S04<br/>Audit: networking<br/>→ LAN design"]
    end

    subgraph W2["Wave 2"]
        S05["S05<br/>Strip dead platforms<br/>(DOS, IPX, modem, WOL)"]
    end

    subgraph W3["Wave 3 — parallel"]
        S06["S06<br/>Compiler compatibility:<br/>game code compiles"]
        S07["S07<br/>Assembly → C++"]
        S08["S08<br/>Platform layer: SDL<br/>video, input, timer"]
        S09["S09<br/>Audio via SDL"]
    end

    subgraph W4["Wave 4"]
        S10["S10<br/>Link & boot to main menu"]:::ms
    end

    subgraph W5["Wave 5"]
        S11["S11<br/>Skirmish vs AI playable"]:::ms
        S12["S12<br/>Portable UDP sockets"]
    end

    subgraph W6["Wave 6 — parallel"]
        S13["S13<br/>LAN discovery, lobby,<br/>game start"]
        S14["S14<br/>Sync & determinism<br/>harness"]
    end

    subgraph W7["Wave 7"]
        S15["S15<br/>LAN release & playtest"]:::ms
    end

    S01 --> S05
    S02 --> S05
    S03 --> S05
    S05 --> S06
    S02 --> S07
    S03 --> S07
    S01 --> S08
    S05 --> S08
    S05 --> S09
    S03 --> S08
    S02 --> S09
    S03 --> S09
    S06 --> S10
    S07 --> S10
    S08 --> S10
    S09 --> S10
    S10 --> S11
    S10 --> S12
    S04 --> S12
    S12 --> S13
    S11 --> S14
    S13 --> S15
    S14 --> S15

    classDef ms fill:#f9a825,color:#000,stroke:#f57f17
    classDef done fill:#2e7d32,color:#fff,stroke:#1b5e20
    class S01,S02,S03,S04 done
```

Green sessions are done. Highlighted sessions end in a milestone: **S10 → M1**, **S11 → M2**, **S15 → M3**.

## Waves and how many agents can work at once

| Wave | Sessions | Parallel agents | Exit condition |
|------|----------|-----------------|----------------|
| 1 | S01, S02, S03, S04 | 4 | Audits written in `docs/codebase/`; CI builds an empty target |
| 2 | S05 | 1 | Dead code removed, decisions recorded |
| 3 | S06, S07, S08, S09 | 4+ (S06 and S07 split further by file group) | Every kept source file compiles |
| 4 | S10 | 1–2 | **M1**: main menu on screen |
| 5 | S11, S12 | 2 | **M2**: skirmish playable; UDP layer portable |
| 6 | S13, S14 | 2 | Two instances play over localhost with sync checks |
| 7 | S15 | 1 + friends | **M3**: LAN games with friends |

## Biggest risks

| Risk | Why it matters | What we do about it |
|------|----------------|--------------------|
| The code assumes 32-bit (`long` = 32 bits, pointer ↔ int casts, `#pragma pack(1)` network structs) | Silent bugs and desyncs on 64-bit | Build 32-bit x86 first (ADR-0003). Go 64-bit later, as its own session. |
| ~340 assembly files in the libraries | No modern assembler accepts TASM/Watcom syntax | S02 finds which ones the Win32 build actually links. S07 ports only those. |
| Determinism breaks during porting | LAN games desync | S14 builds a sync harness. Anything that touches the simulation is flagged in logs. |
| Windows-message-driven sockets (`WSAAsyncSelect`) | Don't work without a Win32 window loop and aren't portable | S12 replaces them with polled non-blocking sockets. |
| Missing proprietary libraries (HMI SOS, Greenleaf) | Code won't link | Greenleaf is modem/DOS only, so it gets deleted (S05). Audio is replaced via SDL (S09). |
