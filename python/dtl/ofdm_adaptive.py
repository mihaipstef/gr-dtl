
def frame_capacity(n_ofdm_syms, ocupied_carriers):
    n_qam_syms = 0
    for i in range(n_ofdm_syms):
        n_qam_syms += len(ocupied_carriers[i%len(ocupied_carriers)])
    return n_qam_syms

def max_bps(cnsts):
    return max([int(c) for c in cnsts])