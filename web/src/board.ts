import { Chessground } from 'chessground';
import type { Api } from 'chessground/api';
import type { Key } from 'chessground/types';
import type { GameSnapshot } from './game';

export class BoardView {
	private readonly cg: Api;
	private pendingPromo: { from: string; to: string } | null = null;

	private lastSnap: GameSnapshot | null = null;

	constructor(
		el: HTMLElement,
		private readonly promoEl: HTMLElement,
		private readonly onUserMove: (from: string, to: string, promotion?: string) => void,
		private readonly needsPromotion: (from: string, to: string) => boolean,
	) {
		this.cg = Chessground(el, {
			coordinates: true,
			animation: { duration: 160 },
			movable: {
				free: false,
				showDests: true,
				events: {
					after: (orig, dest) => this.onDrop(orig, dest),
				},
			},
		});

		this.promoEl.addEventListener('click', (event) => {
			const target = (event.target as HTMLElement).closest('[data-piece]');
			if (!target || !this.pendingPromo) {
				if ((event.target as HTMLElement).dataset.role === 'cancel') {
					this.cancelPromotion();
				}
				return;
			}
			const piece = (target as HTMLElement).dataset.piece;
			if (!piece) {
				return;
			}
			const { from, to } = this.pendingPromo;
			this.pendingPromo = null;
			this.hidePromo();
			this.onUserMove(from, to, piece);
		});
	}

	apply(snap: GameSnapshot): void {
		if (this.pendingPromo) {
			if (!snap.movable || snap.over) {
				this.pendingPromo = null;
				this.hidePromo();
			} else {
				return;
			}
		}
		this.lastSnap = snap;
		const dests = new Map<Key, Key[]>();
		for (const [from, tos] of snap.dests) {
			dests.set(from as Key, tos as Key[]);
		}
		this.cg.set({
			fen: snap.fen,
			orientation: snap.playerColor,
			turnColor: snap.turn,
			check: snap.check,
			lastMove: snap.lastMove
				? ([snap.lastMove.from, snap.lastMove.to] as Key[])
				: undefined,
			movable: {
				color: snap.movable ? snap.playerColor : undefined,
				dests,
			},
		});
	}

	resize(): void {
		this.cg.redrawAll();
	}

	private onDrop(orig: Key, dest: Key): void {
		if (this.needsPromotion(orig, dest)) {
			this.pendingPromo = { from: orig, to: dest };
			this.promoEl.hidden = false;
			return;
		}
		this.onUserMove(orig, dest);
	}

	private cancelPromotion(): void {
		this.pendingPromo = null;
		this.hidePromo();
		if (this.lastSnap) {
			this.apply(this.lastSnap);
		}
	}

	private hidePromo(): void {
		this.promoEl.hidden = true;
	}
}
