#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

use core::slice;
use std::{mem::size_of, os::raw::c_int, usize};

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub fn generate_test<const N: usize>(input_signal: &[[f64; 2]; N]) -> Vec<[f64; 2]> {
    //const SIGNAL_N: usize = N;
    let signal_buf_bytes: usize = size_of::<fftw_complex>() * N;

    let mut output_test = Vec::<[f64; 2]>::new();

    unsafe {
        let input: *mut [f64; 2] = fftw_malloc(signal_buf_bytes).cast();

        let mut input_slice = slice::from_raw_parts_mut(input, N).iter_mut();
        let mut input_signal_iter = input_signal.iter();

        let output: *mut [f64; 2] = fftw_malloc(signal_buf_bytes).cast();
        let plan = fftw_plan_dft_1d(
            N as c_int,
            input,
            output,
            FFTW_BACKWARD as c_int,
            FFTW_ESTIMATE,
        );

        while let (Some(input_slice), Some(input_signal)) =
            (input_slice.next(), input_signal_iter.next())
        {
            *input_slice = *input_signal;
        }

        fftw_execute(plan);

        fftw_destroy_plan(plan);

        /*let output2: *mut [f64; 2] = fftw_malloc(signal_buf_bytes).cast();


        let plan = fftw_plan_dft_1d(
            N as c_int,
            output,
            output2,
            FFTW_FORWARD as c_int,
            FFTW_ESTIMATE,
        );

        fftw_execute(plan);

        fftw_destroy_plan(plan);*/

        let output_vec = slice::from_raw_parts(output, N);

        output_test.extend_from_slice(output_vec);

        fftw_free(input.cast());
        fftw_free(output.cast());
        //fftw_free(output2.cast());
    }

    output_test
}
