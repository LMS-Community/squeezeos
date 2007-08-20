/*
 * SA1111 SAC DMA support
 */

typedef struct {
	volatile u_long SAD_CS;
	volatile dma_addr_t SAD_SA;
	volatile u_long SAD_CA;
	volatile dma_addr_t SAD_SB;
	volatile u_long SAD_CB;
} sac_regs_t;

typedef struct sa1111_dma {
	int (*start)(struct sa1111_dma *, dma_addr_t, size_t);
	void (*reset)(struct sa1111_dma *);
	dma_callback_t callback;
	void *data;
	sac_regs_t *regs;
	int dma_a, dma_b, last_dma;
} sa1111_dma_t;

