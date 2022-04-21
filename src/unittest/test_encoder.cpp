/*
 * Copyright (c) 2021 Vasile Vilvoiu <vasi@vilvoiu.ro>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */
#include "gtest/gtest.h"
#include "ecbor.h"
#include <cmath>
#include <cstring>
#include <vector>
#include <functional>
#include <tuple>
#include <string>
#include <iostream>
#include <limits>

void run_encoder_test_normal(std::function<void(ecbor_encode_context_t*)> callback,
                             std::vector<uint8_t> expected)
{
    // create an encoding context
    constexpr size_t BUFFER_SIZE = 4096;
    uint8_t buf[BUFFER_SIZE];
    ecbor_encode_context_t ctx;
    EXPECT_EQ(ecbor_initialize_encode(&ctx, buf, BUFFER_SIZE), ECBOR_OK);

    callback(&ctx);

    // check output
    auto sizef = ECBOR_GET_ENCODED_BUFFER_SIZE(&ctx);
    size_t sizes;
    EXPECT_EQ(ecbor_get_encoded_buffer_size(&ctx, &sizes), ECBOR_OK);
    EXPECT_EQ(sizef, sizes);
    EXPECT_EQ(sizef, expected.size());

    std::cout << "Act: ";
    for (size_t i = 0; i < sizef; i++) {
        std::cout << std::hex << (uint32_t)ctx.base[i] << std::dec << " ";
    }
    std::cout << std::endl;

    std::cout << "Exp: ";
    for (auto c : expected) {
        std::cout << std::hex << (uint32_t)c << std::dec <<  " ";
    }
    std::cout << std::endl;

    EXPECT_TRUE(std::memcmp(ctx.base, expected.data(), sizef) == 0);
}

TEST(encoder, noop)
{
    run_encoder_test_normal([](ecbor_encode_context_t*) -> void {
            },
            {});
}

TEST(encoder, error_null_parameters)
{
    constexpr size_t BUFFER_SIZE = 4096;
    uint8_t buf[BUFFER_SIZE];

    ecbor_encode_context_t ctx;
    ecbor_item_t item = ecbor_uint(3), foo;

    size_t sz;

    EXPECT_EQ(ecbor_initialize_encode(nullptr, buf, BUFFER_SIZE), ECBOR_ERR_NULL_CONTEXT);
    EXPECT_EQ(ecbor_initialize_encode(&ctx, nullptr, BUFFER_SIZE), ECBOR_ERR_NULL_OUTPUT_BUFFER);
    EXPECT_EQ(ecbor_initialize_encode_streamed(nullptr, buf, BUFFER_SIZE), ECBOR_ERR_NULL_CONTEXT);
    EXPECT_EQ(ecbor_initialize_encode_streamed(&ctx, nullptr, BUFFER_SIZE), ECBOR_ERR_NULL_OUTPUT_BUFFER);

    EXPECT_EQ(ecbor_encode(nullptr, &item), ECBOR_ERR_NULL_CONTEXT);
    EXPECT_EQ(ecbor_encode(&ctx, nullptr), ECBOR_ERR_NULL_ITEM);

    EXPECT_EQ(ecbor_get_encoded_buffer_size(nullptr, &sz), ECBOR_ERR_NULL_CONTEXT);
    EXPECT_EQ(ecbor_get_encoded_buffer_size(&ctx, nullptr), ECBOR_ERR_NULL_PARAMETER);

    EXPECT_EQ(ecbor_array(nullptr, &item, 1), ECBOR_ERR_NULL_ARRAY);
    EXPECT_EQ(ecbor_array(&foo, nullptr, 1), ECBOR_ERR_NULL_ITEM);

    EXPECT_EQ(ecbor_map(nullptr, &item, &item, 1), ECBOR_ERR_NULL_MAP);
    EXPECT_EQ(ecbor_map(&foo, nullptr, &item, 1), ECBOR_ERR_NULL_ITEM);
    EXPECT_EQ(ecbor_map(&foo, &item, nullptr, 1), ECBOR_ERR_NULL_ITEM);
}

TEST(encoder, appendix_a)
{
    using Test = std::tuple<std::string, std::function<void(ecbor_encode_context_t*)>, std::vector<uint8_t>>;

    std::vector<Test> tests = {
        {
            "000",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(0);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x00 }
        },
        {
            "001",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(1);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x01 }
        },
        {
            "002",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(10);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x0a }
        },
        {
            "003",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(23);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x17 }
        },
        {
            "004",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(24);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x18, 0x18 }
        },
        {
            "005",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(25);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x18, 0x19 }
        },
        {
            "006",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(100);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x18, 0x64 }
        },
        {
            "007",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(1000);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x19, 0x03, 0xe8 }
        },
        {
            "008",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(1000000);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x1a, 0x00, 0x0f, 0x42, 0x40 }
        },
        {
            "009",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(1000000000000);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x1b, 0x00, 0x00, 0x00, 0xe8, 0xd4, 0xa5, 0x10, 0x00 }
        },
        {
            "010",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_uint(18446744073709551615ULL);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x1b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
        },
        {
            "011",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<uint8_t> bstr_ = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                ecbor_item_t bstr = ecbor_bstr(bstr_.data(), bstr_.size());
                ecbor_item_t tag = ecbor_tag(&bstr, 2);
                EXPECT_EQ(ecbor_encode(ctx, &tag), ECBOR_OK);
            },
            { 0xc2, 0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
        },
        {
            "012",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_int(0);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x00 } /* NOTE: libecbor cannot produce -0 */
        },
        {
            "013",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<uint8_t> bstr_ = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                ecbor_item_t bstr = ecbor_bstr(bstr_.data(), bstr_.size());
                ecbor_item_t tag = ecbor_tag(&bstr, 3);
                EXPECT_EQ(ecbor_encode(ctx, &tag), ECBOR_OK);
            },
            { 0xc3, 0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
        },
        {
            "014",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_int(-1);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x20 }
        },
        {
            "015",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_int(-10);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x29 }
        },
        {
            "016",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_int(-100);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x38, 0x63 }
        },
        {
            "017",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_int(-1000);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0x39, 0x03, 0xe7 }
        },
        {
            "021",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp64(1.1);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfb, 0x3f, 0xf1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a }
        },
        {
            "024",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp32(1e5);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfa, 0x47, 0xc3, 0x50, 0x00 }
        },
        {
            "025",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp32(340282346638528859811704183484516925440.0f);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfa, 0x7f, 0x7f, 0xff, 0xff }
        },
        {
            "026",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp64(1000000000000000052504760255204420248704468581108159154915854115511802457988908195786371375080447864043704443832883878176942523235360430575644792184786706982848387200926575803737830233794788090059368953234970799945081119038967640880074652742780142494579258788820056842838115669472196386865459400540160.0);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfb, 0x7e, 0x37, 0xe4, 0x3c, 0x88, 0x00, 0x75, 0x9c }
        },
        {
            "030",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp64(-4.1);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfb, 0xc0, 0x10, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66 }
        },
        {
            "034",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp32(std::numeric_limits<float>::infinity());
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfa, 0x7f, 0x80, 0x00, 0x00 }
        },
        {
            "035",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp32(std::nanf(""));
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfa, 0x7f, 0xc0, 0x00, 0x00 }
        },
        {
            "036",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp32(-std::numeric_limits<float>::infinity());
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfa, 0xff, 0x80, 0x00, 0x00 }
        },
        {
            "037",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp64(std::numeric_limits<double>::infinity());
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfb, 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
        },
        {
            "038",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp64(std::nan(""));
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfb, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
        },
        {
            "039",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_fp64(-std::numeric_limits<double>::infinity());
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xfb, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
        },
        {
            "040",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_bool(0);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xf4 }
        },
        {
            "041",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_bool(1);
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xf5 }
        },
        {
            "042",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_null();
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xf6 }
        },
        {
            "043",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t item = ecbor_undefined();
                EXPECT_EQ(ecbor_encode(ctx, &item), ECBOR_OK);
            },
            { 0xf7 }
        },
        {
            "047",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "2013-03-21T20:04:00Z";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                ecbor_item_t tag = ecbor_tag(&str, 0);
                EXPECT_EQ(ecbor_encode(ctx, &tag), ECBOR_OK);
            },
            { 0xc0, 0x74, 0x32, 0x30, 0x31, 0x33, 0x2d, 0x30, 0x33, 0x2d, 0x32, 0x31, 0x54, 0x32, 0x30, 0x3a, 0x30, 0x34, 0x3a, 0x30, 0x30, 0x5a }
        },
        {
            "048",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t val = ecbor_uint(1363896240);
                ecbor_item_t tag = ecbor_tag(&val, 1);
                EXPECT_EQ(ecbor_encode(ctx, &tag), ECBOR_OK);
            },
            { 0xc1, 0x1a, 0x51, 0x4b, 0x67, 0xb0 }
        },
        {
            "049",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t val = ecbor_fp64(1363896240.5);
                ecbor_item_t tag = ecbor_tag(&val, 1);
                EXPECT_EQ(ecbor_encode(ctx, &tag), ECBOR_OK);
            },
            { 0xc1, 0xfb, 0x41, 0xd4, 0x52, 0xd9, 0xec, 0x20, 0x00, 0x00 }
        },
        {
            "050",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<uint8_t> bstr_ = { 0x01, 0x02, 0x03, 0x04 };
                ecbor_item_t bstr = ecbor_bstr(bstr_.data(), bstr_.size());
                ecbor_item_t tag = ecbor_tag(&bstr, 23);
                EXPECT_EQ(ecbor_encode(ctx, &tag), ECBOR_OK);
            },
            { 0xd7, 0x44, 0x01, 0x02, 0x03, 0x04 }
        },
        {
            "051",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<uint8_t> bstr_ = { 0x64, 0x49, 0x45, 0x54, 0x46 };
                ecbor_item_t bstr = ecbor_bstr(bstr_.data(), bstr_.size());
                ecbor_item_t tag = ecbor_tag(&bstr, 24);
                EXPECT_EQ(ecbor_encode(ctx, &tag), ECBOR_OK);
            },
            { 0xd8, 0x18, 0x45, 0x64, 0x49, 0x45, 0x54, 0x46 }
        },
        {
            "052",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "http://www.example.com";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                ecbor_item_t tag = ecbor_tag(&str, 32);
                EXPECT_EQ(ecbor_encode(ctx, &tag), ECBOR_OK);
            },
            { 0xd8, 0x20, 0x76, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d }
        },
        {
            "053",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<uint8_t> bstr_ = {};
                ecbor_item_t bstr = ecbor_bstr(bstr_.data(), bstr_.size());
                EXPECT_EQ(ecbor_encode(ctx, &bstr), ECBOR_OK);
            },
            { 0x40 }
        },
        {
            "054",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<uint8_t> bstr_ = { 0x01, 0x02, 0x03, 0x04 };
                ecbor_item_t bstr = ecbor_bstr(bstr_.data(), bstr_.size());
                EXPECT_EQ(ecbor_encode(ctx, &bstr), ECBOR_OK);
            },
            { 0x44, 0x01, 0x02, 0x03, 0x04 }
        },
        {
            "055",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                EXPECT_EQ(ecbor_encode(ctx, &str), ECBOR_OK);
            },
            { 0x60 }
        },
        {
            "056",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "a";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                EXPECT_EQ(ecbor_encode(ctx, &str), ECBOR_OK);
            },
            { 0x61, 0x61 }
        },
        {
            "057",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "IETF";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                EXPECT_EQ(ecbor_encode(ctx, &str), ECBOR_OK);
            },
            { 0x64, 0x49, 0x45, 0x54, 0x46 }
        },
        {
            "058",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "\"\\";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                EXPECT_EQ(ecbor_encode(ctx, &str), ECBOR_OK);
            },
            { 0x62, 0x22, 0x5c }
        },
        {
            "059",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "ü";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                EXPECT_EQ(ecbor_encode(ctx, &str), ECBOR_OK);
            },
            { 0x62, 0xc3, 0xbc }
        },
        {
            "060",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "水";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                EXPECT_EQ(ecbor_encode(ctx, &str), ECBOR_OK);
            },
            { 0x63, 0xe6, 0xb0, 0xb4 }
        },
        {
            "061",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string str_ = "𐅑";
                ecbor_item_t str = ecbor_str(str_.c_str(), str_.size());
                EXPECT_EQ(ecbor_encode(ctx, &str), ECBOR_OK);
            },
            { 0x64, 0xf0, 0x90, 0x85, 0x91 }
        },
        {
            "062",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t arr;
                EXPECT_EQ(ecbor_array(&arr, nullptr, 0), ECBOR_OK);
                EXPECT_EQ(ecbor_encode(ctx, &arr), ECBOR_OK);
            },
            { 0x80 }
        },
        {
            "063",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<ecbor_item_t> items = {
                    ecbor_uint(1),
                    ecbor_uint(2),
                    ecbor_uint(3)
                };
                ecbor_item_t arr;
                EXPECT_EQ(ecbor_array(&arr, items.data(), items.size()), ECBOR_OK);
                EXPECT_EQ(ecbor_encode(ctx, &arr), ECBOR_OK);
            },
            { 0x83, 0x01, 0x02, 0x03 }
        },
        {
            "064",
            [](ecbor_encode_context_t* ctx) -> void {

                std::vector<ecbor_item_t> i1items = {
                    ecbor_uint(2),
                    ecbor_uint(3)
                };
                ecbor_item_t i1arr;
                EXPECT_EQ(ecbor_array(&i1arr, i1items.data(), i1items.size()), ECBOR_OK);

                std::vector<ecbor_item_t> i2items = {
                    ecbor_uint(4),
                    ecbor_uint(5)
                };
                ecbor_item_t i2arr;
                EXPECT_EQ(ecbor_array(&i2arr, i2items.data(), i2items.size()), ECBOR_OK);

                std::vector<ecbor_item_t> items = {
                    ecbor_uint(1),
                    i1arr,
                    i2arr
                };
                ecbor_item_t arr;
                EXPECT_EQ(ecbor_array(&arr, items.data(), items.size()), ECBOR_OK);
                EXPECT_EQ(ecbor_encode(ctx, &arr), ECBOR_OK);
            },
            { 0x83, 0x01, 0x82, 0x02, 0x03, 0x82, 0x04, 0x05 }
        },
        {
            "065",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<ecbor_item_t> items;
                for (size_t i=1; i<=25; i++) {
                    items.push_back(ecbor_uint(i));
                }
                ecbor_item_t arr;
                EXPECT_EQ(ecbor_array(&arr, items.data(), items.size()), ECBOR_OK);
                EXPECT_EQ(ecbor_encode(ctx, &arr), ECBOR_OK);
            },
            { 0x98, 0x19, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x18, 0x18, 0x19 }
        },
        {
            "066",
            [](ecbor_encode_context_t* ctx) -> void {
                ecbor_item_t map;
                EXPECT_EQ(ecbor_map(&map, nullptr, nullptr, 0), ECBOR_OK);
                EXPECT_EQ(ecbor_encode(ctx, &map), ECBOR_OK);
            },
            { 0xa0 }
        },
        {
            "067",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<ecbor_item_t> keys = {
                    ecbor_uint(1),
                    ecbor_uint(3),
                };
                std::vector<ecbor_item_t> vals = {
                    ecbor_uint(2),
                    ecbor_uint(4),
                };
                EXPECT_EQ(keys.size(), vals.size());
                ecbor_item_t map;

                EXPECT_EQ(ecbor_map(&map, keys.data(), vals.data(), keys.size()), ECBOR_OK);
                EXPECT_EQ(ecbor_encode(ctx, &map), ECBOR_OK);
            },
            { 0xa2, 0x01, 0x02, 0x03, 0x04 }
        },
        {
            "068",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string k1_ = "a";
                std::string k2_ = "b";
                std::vector<ecbor_item_t> keys = {
                    ecbor_str(k1_.c_str(), k1_.size()),
                    ecbor_str(k2_.c_str(), k2_.size()),
                };

                std::vector<ecbor_item_t> arr_ = {
                    ecbor_uint(2),
                    ecbor_uint(3),
                };
                ecbor_item_t arr;
                EXPECT_EQ(ecbor_array(&arr, arr_.data(), arr_.size()), ECBOR_OK);

                std::vector<ecbor_item_t> vals = {
                    ecbor_uint(1),
                    arr
                };
                EXPECT_EQ(keys.size(), vals.size());

                ecbor_item_t map;
                EXPECT_EQ(ecbor_map(&map, keys.data(), vals.data(), keys.size()), ECBOR_OK);
                EXPECT_EQ(ecbor_encode(ctx, &map), ECBOR_OK);
            },
            { 0xa2, 0x61, 0x61, 0x01, 0x61, 0x62, 0x82, 0x02, 0x03 }
        },
        {
            "069",
            [](ecbor_encode_context_t* ctx) -> void {
                std::string k_ = "b";
                std::string v_ = "c";
                ecbor_item_t k = ecbor_str(k_.c_str(), k_.size());
                ecbor_item_t v = ecbor_str(v_.c_str(), v_.size());
                ecbor_item_t map;
                EXPECT_EQ(ecbor_map(&map, &k, &v, 1), ECBOR_OK);

                std::string str_ = "a";
                std::vector<ecbor_item_t> arr_ = {
                    ecbor_str(str_.c_str(), str_.size()),
                    map
                };
                ecbor_item_t arr;
                EXPECT_EQ(ecbor_array(&arr, arr_.data(), arr_.size()), ECBOR_OK);

                EXPECT_EQ(ecbor_encode(ctx, &arr), ECBOR_OK);
            },
            { 0x82, 0x61, 0x61, 0xa1, 0x61, 0x62, 0x61, 0x63 }
        },
        {
            "070",
            [](ecbor_encode_context_t* ctx) -> void {
                std::vector<std::string> keys_ = { "a", "b", "c", "d", "e" };
                std::vector<std::string> vals_ = { "A", "B", "C", "D", "E" };

                std::vector<ecbor_item_t> keys;
                for (auto& k_ : keys_) {
                    keys.push_back(ecbor_str(k_.c_str(), k_.size()));
                }
                std::vector<ecbor_item_t> vals; 
                for (auto& v_ : vals_) {
                    vals.push_back(ecbor_str(v_.c_str(), v_.size()));
                }

                EXPECT_EQ(keys.size(), vals.size());
                ecbor_item_t map;

                EXPECT_EQ(ecbor_map(&map, keys.data(), vals.data(), keys.size()), ECBOR_OK);
                EXPECT_EQ(ecbor_encode(ctx, &map), ECBOR_OK);
            },
            { 0xa5, 0x61, 0x61, 0x61, 0x41, 0x61, 0x62, 0x61, 0x42, 0x61, 0x63, 0x61, 0x43, 0x61, 0x64, 0x61, 0x44, 0x61, 0x65, 0x61, 0x45 }
        },
        /* encoding does not support indefinite strings, maps or arrays */
    };

    size_t idx = 0;
    for (auto& t : tests) {
        std::cout << "Appendix A test " << std::get<0>(t) << " (index " << idx << ")" << std::endl;
        run_encoder_test_normal(std::get<1>(t), std::get<2>(t));
        idx ++;
    }
}
