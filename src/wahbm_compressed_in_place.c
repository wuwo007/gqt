/**
 * @file genotq.c
 * @Author Ryan Layer (ryan.layer@gmail.com)
 * @date May, 2014
 * @brief Functions for converting and opperation on various encoding
 * strategies
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <math.h>
#include <limits.h>
#include <sysexits.h>
#include <err.h>

#include "wahbm_compressed_in_place.h"
#include "wahbm_in_place.h"

#include "timer.h"

// wahbm compressed in place
//{{{ uint32_t wah_compressed_in_place_or(uint32_t *r_wah,
uint32_t wah_compressed_in_place_or(uint32_t *r_wah,
                                         uint32_t r_wah_size,
                                         uint32_t *wah,
                                         uint32_t wah_size)
{
    uint32_t wah_i, wah_v, wah_fill_size, wah_fill_value,
                 r_wah_i, r_wah_v, r_wah_fill_size, r_wah_fill_value,
                 end, num_words;

    r_wah_i = 0;


    for (wah_i = 0; wah_i < wah_size; ++wah_i)
    {
        wah_v = wah[wah_i];
        r_wah_v = r_wah[r_wah_i];

        if (wah_v == 0x80000000)
            abort();
        if (r_wah_v == 0x80000000)
            abort();

        if (wah_v >= 0x80000000) { // wah_v is a fill
            
            wah_fill_value = (wah_v >> 30) & 1;
            wah_fill_size = (wah_v & 0x3fffffff);

            while (wah_fill_size > 0) {

                if (r_wah_v >= 0x80000000) { // r_wah is a fill

                    /*
                    fprintf(stderr, "%u:%s\t%u:%s\n",
                            wah_i,int_to_binary(wah_v),
                            r_wah_i,int_to_binary(r_wah_v));
                    */

                    r_wah_fill_value = (r_wah_v >> 30) & 1;
                    r_wah_fill_size = (r_wah_v & 0x3fffffff);

                    // make a new fill based on the smaller one
                    num_words = MIN(wah_fill_size, r_wah_fill_size);

                    if (num_words > 1) {
                        r_wah[r_wah_i] = (1 << 31) + 
                                        ((r_wah_fill_value | 
                                            wah_fill_value) << 30) + 
                                        num_words;
                    } else {
                        if ((r_wah_fill_value | wah_fill_value) == 1)
                            r_wah[r_wah_i] = 0x7fffffff;
                        else
                            r_wah[r_wah_i] = 0;
                    }

                    r_wah_fill_size -= num_words;
                    wah_fill_size -= num_words;

                    // save any values left on the end of r_wah run
                    if (r_wah_fill_size > 0) {
                        if (r_wah_fill_size == 1) {
                            // we no longer have a fill, write a literal
                            if (r_wah_fill_value == 1) { //all ones
                                r_wah[r_wah_i + num_words] = 0x7fffffff;
                                //fprintf(stderr,"2\n");
                            } else {  // all zeros
                                r_wah[r_wah_i + num_words] = 0;
                                //fprintf(stderr,"3\n");
                            }
                        } else { 
                            // we still have a fill, write it
                            r_wah[r_wah_i + num_words] = 
                                    (1 << 31) + 
                                    (r_wah_fill_value << 30) + 
                                    r_wah_fill_size; 
                            //fprintf(stderr,"4\n");
                        }
                    }

                    r_wah_i += num_words;
                } else { // r_wah is a literal
                    if (wah_fill_value == 1) 
                        r_wah[r_wah_i] = 0x7fffffff;
                    r_wah_i += 1;
                    wah_fill_size -= 1;
                }

                if (r_wah_i < r_wah_size)
                    r_wah_v = r_wah[r_wah_i];

                if (r_wah_v == 0x80000000)
                    abort();
            }
        } else if ( r_wah_v >= 0x80000000) {
            r_wah_fill_value = (r_wah_v >> 30) & 1;

            //wah is not a fill and r_wah is a fill
            //update the current word in r_wah 
            if (r_wah_fill_value == 1) {
                //fill is a one
                //fprintf(stderr,"7\n");
                r_wah[r_wah_i] = 0x7fffffff;
            } else {
                //fill is a zero
                //fprintf(stderr,"8\n");
                r_wah[r_wah_i] = wah_v;
            }

            // we just took one word off the r_wah fill
            r_wah_fill_size = (r_wah_v & 0x3fffffff) - 1;

            if (r_wah_fill_size == 1) {
                // we no longer have a fill, write a literal
                if (r_wah_fill_value == 1) {//all ones
                    //fprintf(stderr,"9\n");
                    r_wah[r_wah_i + 1] = 0x7fffffff;
                } else { // all zeros 
                    //fprintf(stderr,"10\n");
                    r_wah[r_wah_i + 1] = 0;
                }
            } else { 
                // we still have a fill, write it
                //fprintf(stderr,"11\n");
                r_wah[r_wah_i + 1] = (1 << 31) + 
                                     (r_wah_fill_value << 30) + 
                                     r_wah_fill_size; 
            }
            r_wah_i += 1;

        } else {
            //fprintf(stderr,"12\n");
            r_wah[r_wah_i] = r_wah[r_wah_i] | wah[wah_i];
            r_wah_i += 1;
        }
    }

    return r_wah_size;
}
//}}}

//{{{ uint32_t wah_compressed_in_place_and_compressed_in_place(
//Here wer are dealing with two compressed_in_place values
uint32_t wah_compressed_in_place_and_compressed_in_place(
                                          uint32_t *r_wah,
                                          uint32_t r_wah_size,
                                          uint32_t *wah,
                                          uint32_t wah_size)
{
    uint32_t wah_i, wah_v, wah_fill_size, wah_fill_value,
                 r_wah_i, r_wah_v, r_wah_fill_size, r_wah_fill_value,
                 end, num_words;

    r_wah_i = 0;
    uint32_t wah_c = 0;

    for (wah_i = 0; wah_i < wah_size;)
    {
        wah_v = wah[wah_i];
        r_wah_v = r_wah[r_wah_i];

        if (wah_v == 0x80000000)
            abort();
        if (r_wah_v == 0x80000000)
            abort();


        /*
        fprintf(stderr, "r_wah_size:%u\t"
                        "r_wah_i:%u\t"
                        "r_wah_v:%u\t"
                        "wah_i:%u\t" 
                        "wah_v:%u\t" 
                        "wah_size:%u\n",
                        r_wah_size,
                        r_wah_i,
                        r_wah_v,
                        wah_i,
                        wah_v,
                        wah_size);
        */

        if (r_wah_i != wah_i) {
            fprintf(stderr, "r_wah_i:%u\twah_i:%u\n", r_wah_i, wah_i);
            exit(1);
        }

        if (wah_v >= 0x80000000) {
            wah_fill_value = (wah_v >> 30) & 1;
            wah_fill_size = (wah_v & 0x3fffffff);


            while (wah_fill_size > 0) {
                if (r_wah_v >= 0x80000000) { // r_wah is a fill
                    /*
                    fprintf(stderr, "%u:%u\t%u:%u\n", 
                                            wah_i, wah[wah_i],
                                            r_wah_i, r_wah[r_wah_i]);
                    */
                    r_wah_fill_value = (r_wah_v >> 30) & 1;
                    r_wah_fill_size = (r_wah_v & 0x3fffffff);

                    // make a new fill based on the smaller one
                    num_words = MIN(wah_fill_size, r_wah_fill_size);

                    if (num_words == 0)
                        exit(1);

                    if (num_words > 1) {
                        r_wah[r_wah_i] = (1 << 31) + 
                                        ((r_wah_fill_value & 
                                            wah_fill_value) << 30) + 
                                        num_words;
                    } else {
                        if ((r_wah_fill_value & wah_fill_value) == 1) 
                            r_wah[r_wah_i] = 0x7fffffff;
                        else 
                            r_wah[r_wah_i] = 0;
                    }


                    r_wah_fill_size -= num_words;
                    wah_fill_size -= num_words;

                    // save any values left on the end of r_wah run
                    if (r_wah_fill_size > 0) {
                        if (r_wah_fill_size == 1) {
                            // we no longer have a fill, write a literal
                            if (r_wah_fill_value == 1) //all ones
                                r_wah[r_wah_i + num_words] = 0x7fffffff;
                            else  // all zeros
                                r_wah[r_wah_i + num_words] = 0;
                        } else { 
                            // we still have a fill, write it
                            r_wah[r_wah_i + num_words] = 
                                    (1 << 31) + 
                                    (r_wah_fill_value << 30) + 
                                    r_wah_fill_size; 
                        }
                    }

                    wah_i += num_words;
                    r_wah_i += num_words;
                } else { // r_wah is a literal
                    /*
                    fprintf(stderr, "%u:%u\t%u:%u\n", 
                                            wah_i, wah[wah_i],
                                            r_wah_i, r_wah[r_wah_i]);
                    */

                    if (wah_fill_value == 0) 
                        r_wah[r_wah_i] = 0;

                    r_wah_i += 1;
                    wah_i += 1;
                    wah_fill_size -= 1;
                }

                if (r_wah_i < r_wah_size)
                    r_wah_v = r_wah[r_wah_i];
            }


        } else if ( r_wah_v >= 0x80000000) {
            /*
            fprintf(stderr, "%u:%u\t%u:%u\n", 
                                            wah_i, wah[wah_i],
                                            r_wah_i, r_wah[r_wah_i]);
            */
            r_wah_fill_value = (r_wah_v >> 30) & 1;

            //wah is not a fill and r_wah is a fill
            //update the current word in r_wah 
            if (r_wah_fill_value == 1) {
                //fill is a one
                r_wah[r_wah_i] = wah_v;
            } else {
                //fill is a zero
                r_wah[r_wah_i] = 0;
            }


            // we just took one word off the r_wah fill
            r_wah_fill_size = (r_wah_v & 0x3fffffff) - 1;

            if (r_wah_fill_size == 1) {
                // we no longer have a fill, write a literal
                if (r_wah_fill_value == 1) {//all ones
                    r_wah[r_wah_i + 1] = 0x7fffffff;
                } else { // all zeros
                    r_wah[r_wah_i + 1] = 0;
                }
            } else { 
                // we still have a fill, write it
                r_wah[r_wah_i + 1] = (1 << 31) + 
                                     (r_wah_fill_value << 30) + 
                                     r_wah_fill_size; 
            }
            wah_i+=1;
            r_wah_i += 1;

        } else {
            /*
            fprintf(stderr, "%u:%u\t%u:%u\n", 
                                            wah_i, wah[wah_i],
                                            r_wah_i, r_wah[r_wah_i]);
            */

            r_wah[r_wah_i] = r_wah[r_wah_i] & wah[wah_i];
            wah_i += 1;
            r_wah_i += 1;
        }
    }

    return r_wah_size;
}
//}}}

//{{{ uint32_t wah_compressed_in_place_and(uint32_t *r_wah,
//Here wer are dealing with two compressed_in_place values
uint32_t wah_compressed_in_place_and(uint32_t *r_wah,
                                          uint32_t r_wah_size,
                                          uint32_t *wah,
                                          uint32_t wah_size)
{
    uint32_t wah_i, wah_v, wah_fill_size, wah_fill_value,
                 r_wah_i, r_wah_v, r_wah_fill_size, r_wah_fill_value,
                 end, num_words;

    r_wah_i = 0;
    uint32_t wah_c = 0;

    for (wah_i = 0; wah_i < wah_size; ++wah_i)
    {
        wah_v = wah[wah_i];
        r_wah_v = r_wah[r_wah_i];

        if (wah_v == 0x80000000)
            abort();
        if (r_wah_v == 0x80000000)
            abort();

        if (wah_v >= 0x80000000) {
            wah_fill_value = (wah_v >> 30) & 1;
            wah_fill_size = (wah_v & 0x3fffffff);


            while (wah_fill_size > 0) {
                if (r_wah_v >= 0x80000000) { // r_wah is a fill
                    /*
                    fprintf(stderr, "%u:%u\t%u:%u\n", 
                                            wah_i, wah[wah_i],
                                            r_wah_i, r_wah[r_wah_i]);
                    */
                    r_wah_fill_value = (r_wah_v >> 30) & 1;
                    r_wah_fill_size = (r_wah_v & 0x3fffffff);

                    // make a new fill based on the smaller one
                    num_words = MIN(wah_fill_size, r_wah_fill_size);

                    if (num_words == 0)
                        exit(1);

                    if (num_words > 1) {
                        r_wah[r_wah_i] = (1 << 31) + 
                                        ((r_wah_fill_value & 
                                            wah_fill_value) << 30) + 
                                        num_words;
                    } else {
                        if ((r_wah_fill_value & wah_fill_value) == 1) 
                            r_wah[r_wah_i] = 0x7fffffff;
                        else 
                            r_wah[r_wah_i] = 0;
                    }


                    r_wah_fill_size -= num_words;
                    wah_fill_size -= num_words;

                    // save any values left on the end of r_wah run
                    if (r_wah_fill_size > 0) {
                        if (r_wah_fill_size == 1) {
                            // we no longer have a fill, write a literal
                            if (r_wah_fill_value == 1) //all ones
                                r_wah[r_wah_i + num_words] = 0x7fffffff;
                            else  // all zeros
                                r_wah[r_wah_i + num_words] = 0;
                        } else { 
                            // we still have a fill, write it
                            r_wah[r_wah_i + num_words] = 
                                    (1 << 31) + 
                                    (r_wah_fill_value << 30) + 
                                    r_wah_fill_size; 
                        }
                    }

                    r_wah_i += num_words;
                } else { // r_wah is a literal
                    /*
                    fprintf(stderr, "%u:%u\t%u:%u\n", 
                                            wah_i, wah[wah_i],
                                            r_wah_i, r_wah[r_wah_i]);
                    */

                    if (wah_fill_value == 0) 
                        r_wah[r_wah_i] = 0;

                    r_wah_i += 1;
                    wah_fill_size -= 1;
                }

                if (r_wah_i < r_wah_size)
                    r_wah_v = r_wah[r_wah_i];
            }


        } else if ( r_wah_v >= 0x80000000) {
            /*
            fprintf(stderr, "%u:%u\t%u:%u\n", 
                                            wah_i, wah[wah_i],
                                            r_wah_i, r_wah[r_wah_i]);
            */
            r_wah_fill_value = (r_wah_v >> 30) & 1;

            //wah is not a fill and r_wah is a fill
            //update the current word in r_wah 
            if (r_wah_fill_value == 1) {
                //fill is a one
                r_wah[r_wah_i] = wah_v;
            } else {
                //fill is a zero
                r_wah[r_wah_i] = 0;
            }


            // we just took one word off the r_wah fill
            r_wah_fill_size = (r_wah_v & 0x3fffffff) - 1;

            if (r_wah_fill_size == 1) {
                // we no longer have a fill, write a literal
                if (r_wah_fill_value == 1) {//all ones
                    r_wah[r_wah_i + 1] = 0x7fffffff;
                } else { // all zeros
                    r_wah[r_wah_i + 1] = 0;
                }
            } else { 
                // we still have a fill, write it
                r_wah[r_wah_i + 1] = (1 << 31) + 
                                     (r_wah_fill_value << 30) + 
                                     r_wah_fill_size; 
            }
            r_wah_i += 1;

        } else {
            /*
            fprintf(stderr, "%u:%u\t%u:%u\n", 
                                            wah_i, wah[wah_i],
                                            r_wah_i, r_wah[r_wah_i]);
            */

            r_wah[r_wah_i] = r_wah[r_wah_i] & wah[wah_i];
            r_wah_i += 1;
        }
    }

    return r_wah_size;
}
//}}}

//{{{ uint32_t range_records_compressed_in_place_wahbm(struct wah_file wf,
uint32_t range_records_compressed_in_place_wahbm(
            struct wahbm_file *wf,
            uint32_t *record_ids,
            uint32_t num_r,
            uint32_t start_test_value,
            uint32_t end_test_value,
            uint32_t **R) 

{

    //uint32_t max_wah_size = (wf.num_fields + 31 - 1)/ 31;
    uint32_t max_wah_size = (wf->gqt_header->num_variants + 31 - 1)/ 31;
    uint32_t *record_new_bm = (uint32_t *)
                        malloc(sizeof(uint32_t)*max_wah_size);
    if (!record_new_bm )
        err(EX_OSERR, "malloc error");

    uint32_t *or_result_bm = (uint32_t *)
                        malloc(sizeof(uint32_t)*max_wah_size);
    if (!or_result_bm )
        err(EX_OSERR, "malloc error");

    uint32_t *and_result_bm = (uint32_t *)
                        malloc(sizeof(uint32_t)*max_wah_size);
    if (!and_result_bm )
        err(EX_OSERR, "malloc error");
    uint32_t and_result_bm_size = 0,
             record_new_bm_size = 0,
             or_result_bm_size = 0;
    uint32_t i,j;

    //for (i = 0; i < max_wah_size; ++i)
    and_result_bm[0] = (3<<30) + max_wah_size;

    for (i = 0; i < num_r; ++i) {
        // or the appropriate bitmaps
        //memset(or_result_bm, 0, sizeof(uint32_t)*max_wah_size);
        or_result_bm[0] = (2<<30) + max_wah_size; 

        for (j = start_test_value; j < end_test_value; ++j) {

            /*
            record_new_bm_size = get_wah_bitmap_in_place(wf,
                                                         record_ids[i],
                                                         j,
                                                         &record_new_bm);
            */

           record_new_bm_size = get_wahbm_bitmap(wf,
                                                 record_ids[i],
                                                 j,
                                                 &record_new_bm);

           or_result_bm_size = 
                    wah_compressed_in_place_or(or_result_bm,
                                               max_wah_size,
                                               record_new_bm,
                                               record_new_bm_size); 
        }

        // and 
        and_result_bm_size = 
                wah_compressed_in_place_and_compressed_in_place(
                                            and_result_bm,
                                            max_wah_size,
                                            or_result_bm,
                                            or_result_bm_size);

    }

    free(record_new_bm);
    free(or_result_bm);

    *R = and_result_bm;
    return and_result_bm_size;
}
//}}}

//{{{ uint32_t count_range_records_compressed_in_place_wahbm(struct
uint32_t count_range_records_compressed_in_place_wahbm(
            struct wahbm_file *wf,
            uint32_t *record_ids,
            uint32_t num_r,
            uint32_t start_test_value,
            uint32_t end_test_value,
            uint32_t **R) 

{
    //*R = (uint32_t *) calloc(wf.num_fields,sizeof(uint32_t));
    *R = (uint32_t *) calloc(wf->gqt_header->num_variants,sizeof(uint32_t));

    uint32_t max_wah_size = (wf->gqt_header->num_variants + 31 - 1)/ 31;
    uint32_t *record_new_bm = (uint32_t *)
                        malloc(sizeof(uint32_t)*max_wah_size);
    if (!record_new_bm )
        err(EX_OSERR, "malloc error");

    uint32_t *or_result_bm = (uint32_t *)
                        malloc(sizeof(uint32_t)*max_wah_size);
    if (!or_result_bm )
        err(EX_OSERR, "malloc error");

    uint32_t and_result_bm_size = 0,
             record_new_bm_size = 0,
             or_result_bm_size = 0;
    uint32_t i,j,r_size = 0;

#ifdef time_count_range_records_compressed_in_place_wahbm
    unsigned long t1 = 0, t2 = 0, t3 = 0;
#endif 

    for (i = 0; i < num_r; ++i) {
        // or the appropriate bitmaps
        //memset(or_result_bm, 0, sizeof(uint32_t)*max_wah_size);
        or_result_bm[0] = (2<<30) + max_wah_size; 

        for (j = start_test_value; j < end_test_value; ++j) {

#ifdef time_count_range_records_compressed_in_place_wahbm
            start();
#endif
            /*
            record_new_bm_size = get_wah_bitmap_in_place(wf,
                                                         record_ids[i],
                                                         j,
                                                         &record_new_bm);
            */
            record_new_bm_size = get_wahbm_bitmap(wf,
                                                  record_ids[i],
                                                  j,
                                                  &record_new_bm);
#ifdef time_count_range_records_compressed_in_place_wahbm
            stop();
            t1+=report();
#endif

#ifdef time_count_range_records_compressed_in_place_wahbm
            start();
#endif
            or_result_bm_size = 
                    wah_compressed_in_place_or(or_result_bm,
                                               max_wah_size,
                                               record_new_bm,
                                               record_new_bm_size); 
#ifdef time_count_range_records_compressed_in_place_wahbm
            stop();
            t2+=report();
#endif
        }

#ifdef time_count_range_records_compressed_in_place_wahbm
        start();
#endif
        r_size = add_compressed_in_place_wahbm(*R,
                                               wf->gqt_header->num_variants,
                                               or_result_bm,
                                               or_result_bm_size);
#ifdef time_count_range_records_compressed_in_place_wahbm
        stop();
        t3+=report();
#endif


    }

#ifdef time_count_range_records_compressed_in_place_wahbm
    unsigned long tall = t1 + t2 + t3;
    fprintf(stderr,"%lu %f\t%lu %f\t%lu %f\t%lu\n", 
            t1,
            ((double)t1)/((double)tall),
            t2,
            ((double)t2)/((double)tall),
            t3,
            ((double)t3)/((double)tall),
            tall);
#endif
    free(record_new_bm);
    free(or_result_bm);

    return r_size;
}
//}}}

//{{{ uint32_t add_compressed_in_place_wahbm(uint32_t *R,
uint32_t add_compressed_in_place_wahbm(uint32_t *R,
                                       uint32_t r_size,
                                       uint32_t *wah,
                                       uint32_t wah_size)
{

    uint32_t wah_i,
             wah_v,
             num_words,
             fill_bit,
             bits,
             bit,
             bit_i,
             word_i,
             field_i;
    field_i = 0;

    for (wah_i = 0; wah_i < wah_size;) {

        wah_v = wah[wah_i];

        if (wah_v >> 31 == 1) {
            num_words = (wah_v & 0x3fffffff);
            fill_bit = (wah_v>=0xC0000000?1:0);
            bits = (fill_bit?0x7FFFFFFF:0);
        } else {
            num_words = 1;
            bits = wah_v;
        }

        if ( (num_words > 1) && (fill_bit == 0) ) {
            field_i += num_words * 31;
            if (field_i >= r_size)
                return r_size;
        } else {
            for (word_i = 0; word_i < num_words; ++word_i) {
                for (bit_i = 0; bit_i < 31; ++bit_i) {
                    bit = (bits >> (30 - bit_i)) & 1;
                    R[field_i] += bit;
                    field_i += 1;

                    if (field_i >= r_size)
                        return r_size;
                }
            }
        }
        wah_i += num_words;
    }

    return r_size;
}
//}}}

//{{{ uint32_t gt_count_records_compressed_in_place_wahbm(
uint32_t gt_count_records_compressed_in_place_wahbm(struct wahbm_file *wf,
                                                    uint32_t *record_ids,
                                                    uint32_t num_r,
                                                    uint32_t test_value,
                                                    uint32_t **R) 

{
    // TODO: need constants for upper bound.
    return count_range_records_compressed_in_place_wahbm(wf,
                                                         record_ids,
                                                         num_r,
                                                         test_value+1,
                                                         4,
                                                         R);
}
//}}}

//{{{ uint32_t gt_records_compressed_in_place_wahbm(struct wahbm_file *wf,
uint32_t gt_records_compressed_in_place_wahbm(struct wahbm_file *wf,
                                              uint32_t *record_ids,
                                              uint32_t num_r,
                                              uint32_t test_value,
                                              uint32_t **R) 

{
    // TODO: need constants for upper bound.
    return range_records_compressed_in_place_wahbm(wf,
                                                   record_ids,
                                                   num_r,
                                                   test_value+1,
                                                   4,
                                                   R);
}
//}}}

//{{{ uint32_t compressed_in_place_wah_to_ints(uint32_t *W,
uint32_t compressed_in_place_wah_to_ints(uint32_t *W,
                                         uint32_t W_len,
                                         uint32_t **O)
{

    uint32_t wah_i;
    uint32_t num_bits = 0;

    wah_i = 0;
    while (wah_i < W_len) {
        if (W[wah_i] >> 31 == 1)  {
            num_bits += 31 * (W[wah_i] & 0x3fffffff); // zero out the fill bits
            wah_i += (W[wah_i] & 0x3fffffff);
        } else {
            num_bits += 31;
            wah_i += 1;
        }
    }

    uint32_t num_ints = (num_bits + 32 - 1) / 32;
    *O = (uint32_t *) malloc (num_ints * sizeof(uint32_t));
    if (!*O )
        err(EX_OSERR, "malloc error");


    uint32_t num_words,
                 word_i,
                 fill_bit,
                 bits,
                 bit,
                 bit_i,
                 int_i,
                 int_bit_i;

    int_bit_i = 1;
    int_i = 0;
    (*O)[int_i] = 0;
    wah_i = 0;
    while (wah_i < W_len) {

        if (W[wah_i] >> 31 == 1) {
                num_words = (W[wah_i] & 0x3fffffff);
                fill_bit = (W[wah_i]>=0xC0000000?1:0);
                bits = (fill_bit?0x7FFFFFFF:0);
        } else {
            num_words = 1;
            bits = W[wah_i];
        }

        for (word_i = 0; word_i < num_words; ++word_i) {
            for (bit_i = 0; bit_i < 31; ++bit_i) {
                bit = (bits >> (30 - bit_i)) & 1;
                //fprintf(stderr,"%u", bit);

                (*O)[int_i] += bit << (32 - int_bit_i);

                if (int_bit_i == 32) {
                    //fprintf(stderr,"\n%u\n",(*O)[int_i]);
                    int_i += 1;
                    int_bit_i = 0;
                    (*O)[int_i] = 0;
                }

                int_bit_i +=1;
            }
        }

        wah_i += num_words;
    }
    
    return num_ints;
}
//}}}
