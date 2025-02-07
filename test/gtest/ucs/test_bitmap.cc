/**
 * Copyright (c) NVIDIA CORPORATION & AFFILIATES, 2020. ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#include <common/test.h>
#include <ucs/datastruct/bitmap.h>
#include <ucs/datastruct/dynamic_bitmap.h>
#include <ucs/datastruct/static_bitmap.h>

#include <numeric>

class test_ucs_bitmap : public ucs::test {
public:
    virtual void init()
    {
        UCS_BITMAP_CLEAR(&bitmap);
    }

protected:
    void copy_bitmap(ucs_bitmap_t(128) *bitmap, uint64_t *dest)
    {
        int i;

        UCS_BITMAP_FOR_EACH_BIT(*bitmap, i) {
            dest[UCS_BITMAP_WORD_INDEX(*bitmap, i)] |= UCS_BIT(
                    i % UCS_BITMAP_BITS_IN_WORD);
        }
    }

protected:
    ucs_bitmap_t(128) bitmap;
};

void test_set_get_unset(ucs_bitmap_t(128) *bitmap, uint64_t offset)
{
    UCS_BITMAP_SET(*bitmap, offset);
    EXPECT_EQ(UCS_BITMAP_GET(*bitmap, offset), 1);
    EXPECT_EQ(bitmap->bits[offset >= UCS_BITMAP_BITS_IN_WORD], UCS_BIT(offset % 64));
    EXPECT_EQ(bitmap->bits[offset < UCS_BITMAP_BITS_IN_WORD], 0);

    UCS_BITMAP_UNSET(*bitmap, offset);
    EXPECT_EQ(bitmap->bits[0], 0);
    EXPECT_EQ(bitmap->bits[1], 0);
    EXPECT_EQ(UCS_BITMAP_GET(*bitmap, offset), 0);
}

UCS_TEST_F(test_ucs_bitmap, test_popcount) {
    int popcount = UCS_BITMAP_POPCOUNT(bitmap);
    EXPECT_EQ(popcount, 0);
    UCS_BITMAP_SET(bitmap, 12);
    UCS_BITMAP_SET(bitmap, 53);
    UCS_BITMAP_SET(bitmap, 71);
    UCS_BITMAP_SET(bitmap, 110);
    popcount = UCS_BITMAP_POPCOUNT(bitmap);
    EXPECT_EQ(popcount, 4);
}

UCS_TEST_F(test_ucs_bitmap, test_popcount_upto_index) {
    int popcount;
    UCS_BITMAP_SET(bitmap, 17);
    UCS_BITMAP_SET(bitmap, 71);
    UCS_BITMAP_SET(bitmap, 121);
    popcount = UCS_BITMAP_POPCOUNT_UPTO_INDEX(bitmap, 110);
    EXPECT_EQ(popcount, 2);

    popcount = UCS_BITMAP_POPCOUNT_UPTO_INDEX(bitmap, 20);
    EXPECT_EQ(popcount, 1);
}

UCS_TEST_F(test_ucs_bitmap, test_mask) {
    /* coverity[unsigned_compare] */
    UCS_BITMAP_MASK(&bitmap, 0);
    EXPECT_EQ(UCS_BITMAP_IS_ZERO_INPLACE(&bitmap), true);
    UCS_BITMAP_SET(bitmap, 64 + 42);
    UCS_BITMAP_MASK(&bitmap, 64 + 42);

    EXPECT_EQ(bitmap.bits[0], UINT64_MAX);
    EXPECT_EQ(bitmap.bits[1], (1ul << 42) - 1);
}

UCS_TEST_F(test_ucs_bitmap, test_set_all) {
    UCS_BITMAP_SET_ALL(bitmap);
    EXPECT_EQ(bitmap.bits[0], UINT64_MAX);
    EXPECT_EQ(bitmap.bits[1], UINT64_MAX);
}

UCS_TEST_F(test_ucs_bitmap, test_ffs) {
    size_t bit_index;

    bit_index = UCS_BITMAP_FFS(bitmap);
    EXPECT_EQ(bit_index, 128);

    UCS_BITMAP_SET(bitmap, 90);
    UCS_BITMAP_SET(bitmap, 100);
    bit_index = UCS_BITMAP_FFS(bitmap);
    EXPECT_EQ(bit_index, 90);

    UCS_BITMAP_CLEAR(&bitmap);
    UCS_BITMAP_SET(bitmap, 0);
    bit_index = UCS_BITMAP_FFS(bitmap);
    EXPECT_EQ(bit_index, 0);

    UCS_BITMAP_CLEAR(&bitmap);
    UCS_BITMAP_SET(bitmap, 64);
    bit_index = UCS_BITMAP_FFS(bitmap);
    EXPECT_EQ(bit_index, 64);
}

UCS_TEST_F(test_ucs_bitmap, test_is_zero) {
    EXPECT_TRUE(UCS_BITMAP_IS_ZERO_INPLACE(&bitmap));

    UCS_BITMAP_SET(bitmap, 71);
    EXPECT_FALSE(UCS_BITMAP_IS_ZERO_INPLACE(&bitmap));
}

UCS_TEST_F(test_ucs_bitmap, test_get_set_clear)
{
    const uint64_t offset = 15;

    EXPECT_EQ(bitmap.bits[0], 0);
    EXPECT_EQ(bitmap.bits[1], 0);
    EXPECT_EQ(UCS_BITMAP_GET(bitmap, offset), 0);

    test_set_get_unset(&bitmap, offset);
    test_set_get_unset(&bitmap, offset + 64);

    UCS_BITMAP_CLEAR(&bitmap);
    for (int i = 0; i < 128; i++) {
        EXPECT_EQ(UCS_BITMAP_GET(bitmap, i), 0);
    }
}

UCS_TEST_F(test_ucs_bitmap, test_foreach)
{
    uint64_t bitmap_words[2] = {};

    UCS_BITMAP_SET(bitmap, 1);
    UCS_BITMAP_SET(bitmap, 25);
    UCS_BITMAP_SET(bitmap, 61);

    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 0);
    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 37);
    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 58);

    copy_bitmap(&bitmap, bitmap_words);

    EXPECT_EQ(bitmap_words[0], UCS_BIT(1) | UCS_BIT(25) | UCS_BIT(61));
    EXPECT_EQ(bitmap_words[1], UCS_BIT(0) | UCS_BIT(37) | UCS_BIT(58));
}

UCS_TEST_F(test_ucs_bitmap, test_not)
{
    ucs_bitmap_t(128) bitmap2;

    UCS_BITMAP_SET(bitmap, 1);
    bitmap2 = UCS_BITMAP_NOT(bitmap, 128);
    UCS_BITMAP_NOT_INPLACE(&bitmap);

    EXPECT_EQ(bitmap.bits[0], -3ull);
    EXPECT_EQ(bitmap.bits[1], UINT64_MAX);
    EXPECT_EQ(bitmap2.bits[0], -3ull);
    EXPECT_EQ(bitmap2.bits[1], UINT64_MAX);
}

UCS_TEST_F(test_ucs_bitmap, test_and)
{
    ucs_bitmap_t(128) bitmap2, bitmap3;

    UCS_BITMAP_CLEAR(&bitmap2);
    UCS_BITMAP_SET(bitmap, 1);
    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 1);
    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 16);

    UCS_BITMAP_SET(bitmap2, 25);
    UCS_BITMAP_SET(bitmap2, UCS_BITMAP_BITS_IN_WORD + 1);
    UCS_BITMAP_SET(bitmap2, UCS_BITMAP_BITS_IN_WORD + 30);
    bitmap3 = UCS_BITMAP_AND(bitmap, bitmap2, 128);
    UCS_BITMAP_AND_INPLACE(&bitmap, bitmap2);

    EXPECT_EQ(bitmap.bits[0], 0);
    EXPECT_EQ(bitmap.bits[1], UCS_BIT(1));
    EXPECT_EQ(bitmap3.bits[0], 0);
    EXPECT_EQ(bitmap3.bits[1], UCS_BIT(1));
}

UCS_TEST_F(test_ucs_bitmap, test_or)
{
    ucs_bitmap_t(128) bitmap2, bitmap3;

    UCS_BITMAP_CLEAR(&bitmap2);
    UCS_BITMAP_SET(bitmap, 1);
    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 1);
    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 16);

    UCS_BITMAP_SET(bitmap2, 25);
    UCS_BITMAP_SET(bitmap2, UCS_BITMAP_BITS_IN_WORD + 1);
    UCS_BITMAP_SET(bitmap2, UCS_BITMAP_BITS_IN_WORD + 30);
    bitmap3 = UCS_BITMAP_OR(bitmap, bitmap2, 128);
    UCS_BITMAP_OR_INPLACE(&bitmap, bitmap2);

    EXPECT_EQ(bitmap.bits[0], UCS_BIT(1) | UCS_BIT(25));
    EXPECT_EQ(bitmap.bits[1], UCS_BIT(1) | UCS_BIT(16) | UCS_BIT(30));
    EXPECT_EQ(bitmap3.bits[0], UCS_BIT(1) | UCS_BIT(25));
    EXPECT_EQ(bitmap3.bits[1], UCS_BIT(1) | UCS_BIT(16) | UCS_BIT(30));
}


UCS_TEST_F(test_ucs_bitmap, test_xor)
{
    ucs_bitmap_t(128) bitmap2 = UCS_BITMAP_ZERO, bitmap3 = UCS_BITMAP_ZERO;

    bitmap.bits[0]  = 1;
    bitmap.bits[1]  = UINT64_MAX;
    bitmap2.bits[0] = UINT64_MAX;
    bitmap2.bits[1] = 1;
    bitmap3         = UCS_BITMAP_XOR(bitmap, bitmap2, 128);
    UCS_BITMAP_XOR_INPLACE(&bitmap, bitmap2);

    EXPECT_EQ(bitmap.bits[0], -2);
    EXPECT_EQ(bitmap.bits[1], -2);
    EXPECT_EQ(bitmap3.bits[0], -2);
    EXPECT_EQ(bitmap3.bits[1], -2);
}

UCS_TEST_F(test_ucs_bitmap, test_copy)
{
    ucs_bitmap_t(128) bitmap2 = UCS_BITMAP_ZERO;

    UCS_BITMAP_SET(bitmap, 1);
    UCS_BITMAP_SET(bitmap, 25);
    UCS_BITMAP_SET(bitmap, 61);

    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 0);
    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 37);
    UCS_BITMAP_SET(bitmap, UCS_BITMAP_BITS_IN_WORD + 58);

    UCS_BITMAP_COPY(bitmap2, bitmap);

    EXPECT_EQ(bitmap.bits[0], UCS_BIT(1) | UCS_BIT(25) | UCS_BIT(61));
    EXPECT_EQ(bitmap.bits[1], UCS_BIT(0) | UCS_BIT(37) | UCS_BIT(58));
}

UCS_TEST_F(test_ucs_bitmap, test_for_each_bit)
{
    int i = 0, bit_index;
    int bits[128] = {0};

    UCS_BITMAP_SET(bitmap, 0);
    UCS_BITMAP_SET(bitmap, 25);
    UCS_BITMAP_SET(bitmap, 64);
    UCS_BITMAP_SET(bitmap, 100);
    UCS_BITMAP_FOR_EACH_BIT(bitmap, bit_index) {
        i++;
        bits[bit_index]++;
    }

    EXPECT_EQ(i, 4);
    EXPECT_EQ(bits[0], 1);
    EXPECT_EQ(bits[25], 1);
    EXPECT_EQ(bits[64], 1);
    EXPECT_EQ(bits[100], 1);

    /* Test FOREACH on an empty bitmap */
    UCS_BITMAP_CLEAR(&bitmap);
    i = 0;

    UCS_BITMAP_FOR_EACH_BIT(bitmap, bit_index) {
        i++;
    }
    EXPECT_EQ(i, 0);
}

UCS_TEST_F(test_ucs_bitmap, test_for_each_bit_single_word) {
    int i         = 0;
    int bits[128] = {0};
    int bit_index;

    UCS_BITMAP_SET(bitmap, 0);
    UCS_BITMAP_SET(bitmap, 25);
    UCS_BITMAP_FOR_EACH_BIT(bitmap, bit_index) {
        i++;
        bits[bit_index]++;
    }

    EXPECT_EQ(i, 2);
    EXPECT_EQ(bits[0], 1);
    EXPECT_EQ(bits[25], 1);
}

UCS_TEST_F(test_ucs_bitmap, test_compose) {
    /* The result is irrelevant, the code only needs to compile */
    UCS_BITMAP_AND(UCS_BITMAP_NOT(bitmap, 128), bitmap, 128);
}

class test_static_bitmap : public ucs::test {
public:
    virtual void init()
    {
        UCS_STATIC_BITMAP_RESET_ALL(&m_bitmap);
    }

protected:
    using bitmap_t = ucs_static_bitmap_s(128);

    void copy_bitmap(bitmap_t *m_bitmap, uint64_t *dest)
    {
        static const size_t num_words = UCS_STATIC_BITMAP_NUM_WORDS(*m_bitmap);
        ucs_bitmap_bits_copy(m_bitmap->bits, num_words, dest, num_words);
    }

    void test_set_get_unset(uint64_t offset)
    {
        UCS_STATIC_BITMAP_SET(&m_bitmap, offset);
        EXPECT_EQ(1, UCS_STATIC_BITMAP_GET(m_bitmap, offset));
        EXPECT_EQ(m_bitmap.bits[offset >= UCS_BITMAP_BITS_IN_WORD],
                  UCS_BIT(offset % 64));
        EXPECT_EQ(0, m_bitmap.bits[offset < UCS_BITMAP_BITS_IN_WORD]);
        EXPECT_FALSE(UCS_STATIC_BITMAP_IS_ZERO(m_bitmap));

        UCS_STATIC_BITMAP_RESET(&m_bitmap, offset);
        EXPECT_EQ(0, m_bitmap.bits[0]);
        EXPECT_EQ(0, m_bitmap.bits[1]);
        EXPECT_EQ(0, UCS_STATIC_BITMAP_GET(m_bitmap, offset));
    }

protected:
    bitmap_t m_bitmap;

    static void
    set(bitmap_t &bitmap, const std::initializer_list<size_t> &offsets)
    {
        for (auto offset : offsets) {
            UCS_STATIC_BITMAP_SET(&bitmap, offset);
        }
    }
};

UCS_TEST_F(test_static_bitmap, test_popcount) {
    int popcount = UCS_STATIC_BITMAP_POPCOUNT(m_bitmap);
    EXPECT_EQ(0, popcount);
    set(m_bitmap, {12, 53, 71, 110});
    popcount = UCS_STATIC_BITMAP_POPCOUNT(m_bitmap);
    EXPECT_EQ(4, popcount);
}

UCS_TEST_F(test_static_bitmap, test_popcount_upto_index) {
    int popcount;
    set(m_bitmap, {17, 71, 121});
    popcount = UCS_STATIC_BITMAP_POPCOUNT_UPTO_INDEX(m_bitmap, 110);
    EXPECT_EQ(2, popcount);

    popcount = UCS_STATIC_BITMAP_POPCOUNT_UPTO_INDEX(m_bitmap, 20);
    EXPECT_EQ(1, popcount);
}

UCS_TEST_F(test_static_bitmap, test_mask) {
    /* coverity[unsigned_compare] */
    UCS_STATIC_BITMAP_MASK(&m_bitmap, 0);
    EXPECT_TRUE(UCS_STATIC_BITMAP_IS_ZERO(m_bitmap));
    UCS_STATIC_BITMAP_SET(&m_bitmap, 64 + 42);
    UCS_STATIC_BITMAP_MASK(&m_bitmap, 64 + 42);

    EXPECT_EQ(UINT64_MAX, m_bitmap.bits[0]);
    EXPECT_EQ(UCS_MASK(42), m_bitmap.bits[1]);
}

UCS_TEST_F(test_static_bitmap, test_set_all) {
    UCS_STATIC_BITMAP_SET_ALL(&m_bitmap);
    EXPECT_EQ(UINT64_MAX, m_bitmap.bits[0]);
    EXPECT_EQ(UINT64_MAX, m_bitmap.bits[1]);
}

UCS_TEST_F(test_static_bitmap, test_ffs) {
    size_t bit_index;

    bit_index = UCS_STATIC_BITMAP_FFS(m_bitmap);
    EXPECT_EQ(128, bit_index);

    set(m_bitmap, {90, 100});
    bit_index = UCS_STATIC_BITMAP_FFS(m_bitmap);
    EXPECT_EQ(90, bit_index);

    UCS_STATIC_BITMAP_RESET_ALL(&m_bitmap);
    set(m_bitmap, {0});
    bit_index = UCS_STATIC_BITMAP_FFS(m_bitmap);
    EXPECT_EQ(0, bit_index);

    UCS_STATIC_BITMAP_RESET_ALL(&m_bitmap);
    set(m_bitmap, {64});
    bit_index = UCS_STATIC_BITMAP_FFS(m_bitmap);
    EXPECT_EQ(64, bit_index);
}

UCS_TEST_F(test_static_bitmap, test_is_zero) {
    EXPECT_TRUE(UCS_STATIC_BITMAP_IS_ZERO(m_bitmap));

    set(m_bitmap, {71});
    EXPECT_FALSE(UCS_STATIC_BITMAP_IS_ZERO(m_bitmap));
}

UCS_TEST_F(test_static_bitmap, test_get_set_clear)
{
    const uint64_t offset = 15;

    EXPECT_EQ(0, m_bitmap.bits[0]);
    EXPECT_EQ(0, m_bitmap.bits[1]);
    EXPECT_EQ(0, UCS_STATIC_BITMAP_GET(m_bitmap, offset));

    test_set_get_unset(offset);
    test_set_get_unset(offset + 64);

    UCS_STATIC_BITMAP_RESET_ALL(&m_bitmap);
    for (int i = 0; i < 128; i++) {
        EXPECT_EQ(0, UCS_STATIC_BITMAP_GET(m_bitmap, i));
    }
}

UCS_TEST_F(test_static_bitmap, test_foreach)
{
    uint64_t bitmap_words[2] = {};

    set(m_bitmap, {1, 25, 61});

    UCS_STATIC_BITMAP_SET(&m_bitmap, UCS_BITMAP_BITS_IN_WORD + 0);
    UCS_STATIC_BITMAP_SET(&m_bitmap, UCS_BITMAP_BITS_IN_WORD + 37);
    UCS_STATIC_BITMAP_SET(&m_bitmap, UCS_BITMAP_BITS_IN_WORD + 58);

    copy_bitmap(&m_bitmap, bitmap_words);

    EXPECT_EQ(UCS_BIT(1) | UCS_BIT(25) | UCS_BIT(61), bitmap_words[0]);
    EXPECT_EQ(UCS_BIT(0) | UCS_BIT(37) | UCS_BIT(58), bitmap_words[1]);
}

UCS_TEST_F(test_static_bitmap, test_not)
{
    bitmap_t bitmap2;

    set(m_bitmap, {1});
    bitmap2 = UCS_STATIC_BITMAP_NOT(m_bitmap);
    UCS_STATIC_BITMAP_NOT_INPLACE(&m_bitmap);

    EXPECT_EQ(-3ull, m_bitmap.bits[0]);
    EXPECT_EQ(UINT64_MAX, m_bitmap.bits[1]);
    EXPECT_EQ(-3ull, bitmap2.bits[0]);
    EXPECT_EQ(UINT64_MAX, bitmap2.bits[1]);
}

UCS_TEST_F(test_static_bitmap, test_and)
{
    bitmap_t bitmap2, bitmap3;

    UCS_STATIC_BITMAP_RESET_ALL(&bitmap2);
    set(m_bitmap,
        {1, UCS_BITMAP_BITS_IN_WORD + 1, UCS_BITMAP_BITS_IN_WORD + 16});
    set(bitmap2,
        {25, UCS_BITMAP_BITS_IN_WORD + 1, UCS_BITMAP_BITS_IN_WORD + 30});
    bitmap3 = UCS_STATIC_BITMAP_AND(m_bitmap, bitmap2);
    UCS_STATIC_BITMAP_AND_INPLACE(&m_bitmap, bitmap2);

    EXPECT_EQ(0, m_bitmap.bits[0]);
    EXPECT_EQ(UCS_BIT(1), m_bitmap.bits[1]);
    EXPECT_EQ(0, bitmap3.bits[0]);
    EXPECT_EQ(UCS_BIT(1), bitmap3.bits[1]);
}

UCS_TEST_F(test_static_bitmap, test_or)
{
    bitmap_t bitmap2, bitmap3;

    UCS_STATIC_BITMAP_RESET_ALL(&bitmap2);
    set(m_bitmap,
        {1, UCS_BITMAP_BITS_IN_WORD + 1, UCS_BITMAP_BITS_IN_WORD + 16});
    set(bitmap2,
        {25, UCS_BITMAP_BITS_IN_WORD + 1, UCS_BITMAP_BITS_IN_WORD + 30});
    bitmap3 = UCS_STATIC_BITMAP_OR(m_bitmap, bitmap2);
    UCS_STATIC_BITMAP_OR_INPLACE(&m_bitmap, bitmap2);

    EXPECT_EQ(UCS_BIT(1) | UCS_BIT(25), m_bitmap.bits[0]);
    EXPECT_EQ(UCS_BIT(1) | UCS_BIT(16) | UCS_BIT(30), m_bitmap.bits[1]);
    EXPECT_EQ(UCS_BIT(1) | UCS_BIT(25), bitmap3.bits[0]);
    EXPECT_EQ(UCS_BIT(1) | UCS_BIT(16) | UCS_BIT(30), bitmap3.bits[1]);
}

UCS_TEST_F(test_static_bitmap, test_xor) {
    bitmap_t bitmap2 = UCS_STATIC_BITMAP_ZERO_INITIALIZER,
             bitmap3 = UCS_STATIC_BITMAP_ZERO_INITIALIZER;

    m_bitmap.bits[0] = 1;
    m_bitmap.bits[1] = UINT64_MAX;
    bitmap2.bits[0]  = UINT64_MAX;
    bitmap2.bits[1]  = 1;
    bitmap3          = UCS_STATIC_BITMAP_XOR(m_bitmap, bitmap2);
    UCS_STATIC_BITMAP_XOR_INPLACE(&m_bitmap, bitmap2);

    EXPECT_EQ(-2, m_bitmap.bits[0]);
    EXPECT_EQ(-2, m_bitmap.bits[1]);
    EXPECT_EQ(-2, bitmap3.bits[0]);
    EXPECT_EQ(-2, bitmap3.bits[1]);
}

UCS_TEST_F(test_static_bitmap, test_copy)
{
    bitmap_t bitmap2 = UCS_STATIC_BITMAP_ZERO_INITIALIZER;

    set(m_bitmap, {1, 25, 61, UCS_BITMAP_BITS_IN_WORD,
                   UCS_BITMAP_BITS_IN_WORD + 37, UCS_BITMAP_BITS_IN_WORD + 58});

    UCS_STATIC_BITMAP_COPY(&bitmap2, m_bitmap);

    EXPECT_EQ(UCS_BIT(1) | UCS_BIT(25) | UCS_BIT(61), m_bitmap.bits[0]);
    EXPECT_EQ(UCS_BIT(0) | UCS_BIT(37) | UCS_BIT(58), m_bitmap.bits[1]);
}

UCS_TEST_F(test_static_bitmap, test_for_each_bit)
{
    std::vector<int> bits;
    int bit_index;

    set(m_bitmap, {0, 25, 64, 100});
    UCS_STATIC_BITMAP_FOR_EACH_BIT(bit_index, &m_bitmap) {
        bits.push_back(bit_index);
    }

    EXPECT_EQ(4u, bits.size());
    EXPECT_EQ(0, bits[0]);
    EXPECT_EQ(25, bits[1]);
    EXPECT_EQ(64, bits[2]);
    EXPECT_EQ(100, bits[3]);

    /* Test FOREACH on an empty m_bitmap */
    UCS_STATIC_BITMAP_RESET_ALL(&m_bitmap);

    bits.clear();
    UCS_STATIC_BITMAP_FOR_EACH_BIT(bit_index, &m_bitmap) {
        bits.push_back(bit_index);
    }
    EXPECT_TRUE(bits.empty());
}

UCS_TEST_F(test_static_bitmap, test_for_each_bit_single_word) {
    std::vector<int> bits;
    int bit_index;

    set(m_bitmap, {0, 25, 104});
    UCS_STATIC_BITMAP_FOR_EACH_BIT(bit_index, &m_bitmap) {
        bits.push_back(bit_index);
    }

    EXPECT_EQ(3, bits.size());
    EXPECT_EQ(0, bits[0]);
    EXPECT_EQ(25, bits[1]);
    EXPECT_EQ(104, bits[2]);
}

UCS_TEST_F(test_static_bitmap, test_compose) {
    /* The result is irrelevant, the code only needs to compile */
    UCS_STATIC_BITMAP_AND(UCS_STATIC_BITMAP_NOT(m_bitmap), m_bitmap);
}

class test_dynamic_bitmap : public ucs::test {
public:
    virtual void init()
    {
        ucs_dynamic_bitmap_init(&m_bitmap);
    }

    virtual void cleanup()
    {
        ucs_dynamic_bitmap_cleanup(&m_bitmap);
    }

protected:
    ucs_dynamic_bitmap_t m_bitmap;

    static void
    set(ucs_dynamic_bitmap_t &bitmap, const std::vector<size_t> &offsets);

    void expect_bits(const std::vector<size_t> &offsets) const;

    void test_ffs_fns(const std::vector<size_t> &offsets) const;
};

void test_dynamic_bitmap::set(ucs_dynamic_bitmap_t &bitmap,
                              const std::vector<size_t> &offsets)
{
    for (auto offset : offsets) {
        ucs_dynamic_bitmap_set(&bitmap, offset);
    }
}

void test_dynamic_bitmap::expect_bits(const std::vector<size_t> &offsets) const
{
    EXPECT_EQ(offsets.size(), ucs_dynamic_bitmap_popcount(&m_bitmap));
    for (auto offset : offsets) {
        EXPECT_EQ(1, ucs_dynamic_bitmap_get(&m_bitmap, offset));
    }
}

void test_dynamic_bitmap::test_ffs_fns(const std::vector<size_t> &offsets) const
{
    ucs_dynamic_bitmap_t bitmap;

    ucs_dynamic_bitmap_init(&bitmap);
    set(bitmap, offsets);

    EXPECT_EQ(offsets.size(), ucs_dynamic_bitmap_popcount(&bitmap));

    if (!offsets.empty()) {
        /* Asking more bits that are set should result in a value larger than
           bitmap size */
        EXPECT_GT(ucs_dynamic_bitmap_fns(&bitmap, offsets.size()),
                  offsets.back());

        /* Verify first set bit */
        EXPECT_EQ(offsets.front(), ucs_dynamic_bitmap_ffs(&bitmap));
        EXPECT_EQ(offsets.front(), ucs_dynamic_bitmap_fns(&bitmap, 0));

        /* Verify all set bits */
        size_t count = 0;
        for (auto bit_index : offsets) {
            EXPECT_EQ(bit_index, ucs_dynamic_bitmap_fns(&bitmap, count));
            ++count;
        }
    }

    ucs_dynamic_bitmap_cleanup(&bitmap);
}


UCS_TEST_F(test_dynamic_bitmap, test_get_set_reset)
{
    const size_t bit_index = 1237;

    EXPECT_TRUE(ucs_dynamic_bitmap_is_zero(&m_bitmap));

    EXPECT_EQ(0, ucs_dynamic_bitmap_get(&m_bitmap, bit_index));
    ucs_dynamic_bitmap_set(&m_bitmap, bit_index);
    EXPECT_EQ(1, ucs_dynamic_bitmap_get(&m_bitmap, bit_index));
    ucs_dynamic_bitmap_reset(&m_bitmap, bit_index);
    EXPECT_EQ(0, ucs_dynamic_bitmap_get(&m_bitmap, bit_index));

    EXPECT_TRUE(ucs_dynamic_bitmap_is_zero(&m_bitmap));
}

UCS_TEST_F(test_dynamic_bitmap, test_popcount)
{
    EXPECT_EQ(0, ucs_dynamic_bitmap_popcount(&m_bitmap));
    ucs_dynamic_bitmap_set(&m_bitmap, 12);
    EXPECT_EQ(1, ucs_dynamic_bitmap_popcount(&m_bitmap));
    ucs_dynamic_bitmap_set(&m_bitmap, 53);
    EXPECT_EQ(2, ucs_dynamic_bitmap_popcount(&m_bitmap));
    ucs_dynamic_bitmap_set(&m_bitmap, 71);
    EXPECT_EQ(3, ucs_dynamic_bitmap_popcount(&m_bitmap));
    ucs_dynamic_bitmap_set(&m_bitmap, 110);
    EXPECT_EQ(4, ucs_dynamic_bitmap_popcount(&m_bitmap));
}

UCS_TEST_F(test_dynamic_bitmap, test_popcount_upto_index)
{
    int popcount;
    set(m_bitmap, {17, 71, 121});
    popcount = ucs_dynamic_bitmap_popcount_upto_index(&m_bitmap, 110);
    EXPECT_EQ(2, popcount);

    popcount = ucs_dynamic_bitmap_popcount_upto_index(&m_bitmap, 121);
    EXPECT_EQ(2, popcount);

    popcount = ucs_dynamic_bitmap_popcount_upto_index(&m_bitmap, 20);
    EXPECT_EQ(1, popcount);
}

UCS_TEST_F(test_dynamic_bitmap, test_ffs_fns) {
    test_ffs_fns({120, 121, 127, 128, 712});
    test_ffs_fns({});

    std::vector<size_t> vec(128);
    std::iota(vec.begin(), vec.end(), 0);
    test_ffs_fns(vec);
}

UCS_TEST_F(test_dynamic_bitmap, test_for_each_bit)
{
    set(m_bitmap, {0, 25, 64, 100});

    std::vector<int> bits;
    int bit_index;
    UCS_DYNAMIC_BITMAP_FOR_EACH_BIT(bit_index, &m_bitmap) {
        bits.push_back(bit_index);
    }

    EXPECT_EQ(4, bits.size());
    EXPECT_EQ(0, bits[0]);
    EXPECT_EQ(25, bits[1]);
    EXPECT_EQ(64, bits[2]);
    EXPECT_EQ(100, bits[3]);

    /* Test FOREACH on an empty m_bitmap */
    ucs_dynamic_bitmap_reset_all(&m_bitmap);
    bits.clear();
    UCS_DYNAMIC_BITMAP_FOR_EACH_BIT(bit_index, &m_bitmap) {
        bits.push_back(bit_index);
    }
    EXPECT_TRUE(bits.empty());
}

UCS_TEST_F(test_dynamic_bitmap, is_equal)
{
    ucs_dynamic_bitmap_t bitmap2;
    ucs_dynamic_bitmap_init(&bitmap2);

    EXPECT_TRUE(ucs_dynamic_bitmap_is_equal(&m_bitmap, &m_bitmap));
    EXPECT_TRUE(ucs_dynamic_bitmap_is_equal(&m_bitmap, &bitmap2));

    ucs_dynamic_bitmap_set(&m_bitmap, 17);
    EXPECT_FALSE(ucs_dynamic_bitmap_is_equal(&m_bitmap, &bitmap2));

    ucs_dynamic_bitmap_reset(&m_bitmap, 17);
    EXPECT_TRUE(ucs_dynamic_bitmap_is_equal(&m_bitmap, &bitmap2));

    ucs_dynamic_bitmap_set(&m_bitmap, 17);
    ucs_dynamic_bitmap_set(&bitmap2, 17);
    EXPECT_TRUE(ucs_dynamic_bitmap_is_equal(&m_bitmap, &bitmap2));

    ucs_dynamic_bitmap_cleanup(&bitmap2);
}

UCS_TEST_F(test_dynamic_bitmap, test_not)
{
    set(m_bitmap, {1, 3, 4});
    EXPECT_EQ(3, ucs_dynamic_bitmap_popcount(&m_bitmap));

    ucs_dynamic_bitmap_not_inplace(&m_bitmap);
    EXPECT_EQ(1, ucs_dynamic_bitmap_get(&m_bitmap, 0));
    EXPECT_EQ(0, ucs_dynamic_bitmap_get(&m_bitmap, 1));
    EXPECT_EQ(1, ucs_dynamic_bitmap_get(&m_bitmap, 2));
    EXPECT_EQ(0, ucs_dynamic_bitmap_get(&m_bitmap, 3));

    EXPECT_EQ(UCS_BITMAP_BITS_IN_WORD - 3,
              ucs_dynamic_bitmap_popcount(&m_bitmap));
}

UCS_TEST_F(test_dynamic_bitmap, test_and)
{
    ucs_dynamic_bitmap_t bitmap2;
    ucs_dynamic_bitmap_init(&bitmap2);

    set(m_bitmap, {1, 3, 4});
    set(bitmap2, {2, 3, 4, 5, 2000});

    ucs_dynamic_bitmap_and_inplace(&m_bitmap, &bitmap2);
    expect_bits({3, 4});

    ucs_dynamic_bitmap_cleanup(&bitmap2);
}

UCS_TEST_F(test_dynamic_bitmap, test_or)
{
    ucs_dynamic_bitmap_t bitmap2;
    ucs_dynamic_bitmap_init(&bitmap2);

    set(m_bitmap, {1, 3, 4});
    set(bitmap2, {2, 3, 4, 5, 1000});

    ucs_dynamic_bitmap_or_inplace(&m_bitmap, &bitmap2);
    expect_bits({1, 2, 3, 4, 5, 1000});

    ucs_dynamic_bitmap_cleanup(&bitmap2);
}

UCS_TEST_F(test_dynamic_bitmap, test_xor)
{
    ucs_dynamic_bitmap_t bitmap2;
    ucs_dynamic_bitmap_init(&bitmap2);

    set(m_bitmap, {1, 3, 4});
    set(bitmap2, {2, 3, 4, 5, 1200});

    ucs_dynamic_bitmap_xor_inplace(&m_bitmap, &bitmap2);
    expect_bits({1, 2, 5, 1200});

    ucs_dynamic_bitmap_cleanup(&bitmap2);
}
