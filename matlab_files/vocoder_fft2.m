function y = vocoder_fft2(carr, mod, bands, sr)

    len_total = min(length(carr), length(mod));
    carrier = carr(1:len_total);
    modulator = mod(1:len_total);
    num_samples = 512;
    y = zeros(len_total, 1);
    k = 1;
    while ((k+num_samples-1) <= len_total)
        
        % Calculating start and end index for samples
        begin_i = k;
        end_i = min(len_total, k + num_samples - 1);
        samples_in_window = end_i - begin_i + 1;
        
        % Calculating FFT
        fft_car = fft(carrier(begin_i:end_i) .* hamming(samples_in_window));
        fft_mod = fft(modulator(begin_i:end_i) .* hamming(samples_in_window));
        
        len = length(fft_mod);
        mod_mags = zeros(1, bands);
        freq_bin = sr/len; % Frequency in each bin
        F_limit = sr; % The top frequency from where the bands will be divided
        freq_per_band = F_limit/bands;
        bins_per_band = floor(freq_per_band/freq_bin);
        
        for i = 1:bands
            temp_mag = 0;
            for j = 1:bins_per_band
                index = (i-1)*bins_per_band + j;
                temp_mag = temp_mag + abs(fft_mod(index));
            end
            mod_mags(i) = temp_mag/bins_per_band;
        end

        for i = 1:samples_in_window
            index = min(bands, ceil(i/bins_per_band));
            fft_car(i) = fft_car(i)*mod_mags(index);
        end

        vocode_out = real(ifft(fft_car));
        %display(size(vocode_out));
        %display(size(begin_i:end_i));
        y(begin_i:end_i) = y(begin_i:end_i) + vocode_out;
        
        k = k + num_samples/2;

    end

    y = y/max(abs(y));