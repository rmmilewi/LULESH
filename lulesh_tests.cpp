#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <cstring>
#include "lulesh.h"

#define USE_MPI 0

class LuleshTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(LuleshTest, StrToIntValidInteger) {
    Int_t result;
    int status = StrToInt("123", &result);
    EXPECT_EQ(status, 1);
    EXPECT_EQ(result, 123);
}

TEST_F(LuleshTest, StrToIntValidIntegerWithSpace) {
    Int_t result;
    int status = StrToInt("456 ", &result);
    EXPECT_EQ(status, 1);
    EXPECT_EQ(result, 456);
}

TEST_F(LuleshTest, StrToIntNullPointer) {
    Int_t result = 42;
    int status = StrToInt(nullptr, &result);
    EXPECT_EQ(status, 0);
    EXPECT_EQ(result, 42);
}

TEST_F(LuleshTest, StrToIntEmptyString) {
    Int_t result;
    int status = StrToInt("", &result);
    EXPECT_EQ(status, 0);
}

TEST_F(LuleshTest, StrToIntNonNumeric) {
    Int_t result;
    int status = StrToInt("abc", &result);
    EXPECT_EQ(status, 0);
}

TEST_F(LuleshTest, StrToIntMixedContent) {
    Int_t result;
    int status = StrToInt("123abc", &result);
    EXPECT_EQ(status, 0);
}

TEST_F(LuleshTest, StrToIntNegativeNumber) {
    Int_t result;
    int status = StrToInt("-789", &result);
    EXPECT_EQ(status, 1);
    EXPECT_EQ(result, -789);
}

TEST_F(LuleshTest, StrToIntMaxValue) {
    Int_t result;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", std::numeric_limits<Int_t>::max());
    int status = StrToInt(buffer, &result);
    EXPECT_EQ(status, 1);
    EXPECT_EQ(result, std::numeric_limits<Int_t>::max());
}

TEST_F(LuleshTest, StrToIntMinValue) {
    Int_t result;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", std::numeric_limits<Int_t>::min());
    int status = StrToInt(buffer, &result);
    EXPECT_EQ(status, 1);
    EXPECT_EQ(result, std::numeric_limits<Int_t>::min());
}

TEST_F(LuleshTest, StrToIntOverflow) {
    Int_t result;
    std::string overflowStr = "9999999999999999999";
    int status = StrToInt(overflowStr.c_str(), &result);
    EXPECT_EQ(status, 1);
}

TEST_F(LuleshTest, SQRTFloat) {
    real4 input = 16.0f;
    real4 result = SQRT(input);
    EXPECT_FLOAT_EQ(result, 4.0f);
}

TEST_F(LuleshTest, SQRTDouble) {
    real8 input = 25.0;
    real8 result = SQRT(input);
    EXPECT_DOUBLE_EQ(result, 5.0);
}

TEST_F(LuleshTest, SQRTLongDouble) {
    real10 input = 36.0L;
    real10 result = SQRT(input);
    EXPECT_DOUBLE_EQ(static_cast<double>(result), 6.0);
}

TEST_F(LuleshTest, SQRTZero) {
    real8 input = 0.0;
    real8 result = SQRT(input);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(LuleshTest, CBRTFloat) {
    real4 input = 8.0f;
    real4 result = CBRT(input);
    EXPECT_FLOAT_EQ(result, 2.0f);
}

TEST_F(LuleshTest, CBRTDouble) {
    real8 input = 27.0;
    real8 result = CBRT(input);
    EXPECT_DOUBLE_EQ(result, 3.0);
}

TEST_F(LuleshTest, CBRTLongDouble) {
    real10 input = 125.0L;
    real10 result = CBRT(input);
    EXPECT_DOUBLE_EQ(static_cast<double>(result), 5.0);
}

TEST_F(LuleshTest, CBRTNegative) {
    real8 input = -27.0;
    real8 result = CBRT(input);
    EXPECT_DOUBLE_EQ(result, -3.0);
}

TEST_F(LuleshTest, CBRTZero) {
    real8 input = 0.0;
    real8 result = CBRT(input);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(LuleshTest, FABSFloat) {
    real4 input = -16.0f;
    real4 result = FABS(input);
    EXPECT_FLOAT_EQ(result, 16.0f);
}

TEST_F(LuleshTest, FABSDouble) {
    real8 input = -25.0;
    real8 result = FABS(input);
    EXPECT_DOUBLE_EQ(result, 25.0);
}

TEST_F(LuleshTest, FABSLongDouble) {
    real10 input = -36.0L;
    real10 result = FABS(input);
    EXPECT_DOUBLE_EQ(static_cast<double>(result), 36.0);
}

TEST_F(LuleshTest, FABSZero) {
    real8 input = 0.0;
    real8 result = FABS(input);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(LuleshTest, FABSPositive) {
    real8 input = 42.0;
    real8 result = FABS(input);
    EXPECT_DOUBLE_EQ(result, 42.0);
}

TEST_F(LuleshTest, AllocateAndRelease) {
    const size_t size = 10;
    Real_t* ptr = Allocate<Real_t>(size);
    ASSERT_NE(ptr, nullptr);
    
    for (size_t i = 0; i < size; ++i) {
        ptr[i] = static_cast<Real_t>(i);
    }
    
    for (size_t i = 0; i < size; ++i) {
        EXPECT_DOUBLE_EQ(ptr[i], static_cast<Real_t>(i));
    }
    
    Release(&ptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(LuleshTest, ReleaseNullPointer) {
    Real_t* ptr = nullptr;
    Release(&ptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(LuleshTest, CalcElemVolumePositiveCube) {
    Real_t x[8] = {0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0};
    Real_t y[8] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0};
    Real_t z[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
    
    Real_t volume = CalcElemVolume(x, y, z);
    EXPECT_DOUBLE_EQ(volume, 1.0);
}

TEST_F(LuleshTest, CalcElemVolumeNegativeCube) {
    Real_t x[8] = {0.0, -1.0, -1.0, 0.0, 0.0, -1.0, -1.0, 0.0};
    Real_t y[8] = {0.0, 0.0, -1.0, -1.0, 0.0, 0.0, -1.0, -1.0};
    Real_t z[8] = {0.0, 0.0, 0.0, 0.0, -1.0, -1.0, -1.0, -1.0};
    
    Real_t volume = CalcElemVolume(x, y, z);
    EXPECT_DOUBLE_EQ(volume, 1.0);
}

TEST_F(LuleshTest, CalcElemVolumeRectangularPrism) {
    Real_t x[8] = {0.0, 2.0, 2.0, 0.0, 0.0, 2.0, 2.0, 0.0};
    Real_t y[8] = {0.0, 0.0, 3.0, 3.0, 0.0, 0.0, 3.0, 3.0};
    Real_t z[8] = {0.0, 0.0, 0.0, 0.0, 4.0, 4.0, 4.0, 4.0};
    
    Real_t volume = CalcElemVolume(x, y, z);
    EXPECT_DOUBLE_EQ(volume, 24.0);
}

TEST_F(LuleshTest, CalcElemVolumeZeroVolume) {
    Real_t x[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    Real_t y[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    Real_t z[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    
    Real_t volume = CalcElemVolume(x, y, z);
    EXPECT_DOUBLE_EQ(volume, 0.0);
}

TEST_F(LuleshTest, CalcElemVolumeDistortedElement) {
    Real_t x[8] = {0.0, 1.0, 1.0, 0.0, 0.5, 1.5, 1.5, 0.5};
    Real_t y[8] = {0.0, 0.0, 1.0, 1.0, 0.5, 0.5, 1.5, 1.5};
    Real_t z[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
    
    Real_t volume = CalcElemVolume(x, y, z);
    EXPECT_GT(volume, 0.0);
}

TEST_F(LuleshTest, CalcElemVolumeInvertedElement) {
    Real_t x[8] = {0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0};
    Real_t y[8] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0};
    Real_t z[8] = {1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0};
    
    Real_t volume = CalcElemVolume(x, y, z);
    EXPECT_LT(volume, 0.0);
}

TEST_F(LuleshTest, ParseCommandLineOptionsDefault) {
    char* argv[] = {(char*)"lulesh"};
    int argc = 1;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.its, 0);
    EXPECT_EQ(opts.nx, 0);
    EXPECT_EQ(opts.numReg, 0);
    EXPECT_EQ(opts.numFiles, 0);
    EXPECT_EQ(opts.showProg, 0);
    EXPECT_EQ(opts.quiet, 0);
    EXPECT_EQ(opts.viz, 0);
    EXPECT_EQ(opts.cost, 0);
    EXPECT_EQ(opts.balance, 0);
}

TEST_F(LuleshTest, ParseCommandLineOptionsIterations) {
    char* argv[] = {(char*)"lulesh", (char*)"-i", (char*)"100"};
    int argc = 3;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.its, 100);
}

TEST_F(LuleshTest, ParseCommandLineOptionsSize) {
    char* argv[] = {(char*)"lulesh", (char*)"-s", (char*)"50"};
    int argc = 3;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.nx, 50);
}

TEST_F(LuleshTest, ParseCommandLineOptionsRegions) {
    char* argv[] = {(char*)"lulesh", (char*)"-r", (char*)"20"};
    int argc = 3;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.numReg, 20);
}

TEST_F(LuleshTest, ParseCommandLineOptionsFiles) {
    char* argv[] = {(char*)"lulesh", (char*)"-f", (char*)"5"};
    int argc = 3;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.numFiles, 5);
}

TEST_F(LuleshTest, ParseCommandLineOptionsProgress) {
    char* argv[] = {(char*)"lulesh", (char*)"-p"};
    int argc = 2;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.showProg, 1);
}

TEST_F(LuleshTest, ParseCommandLineOptionsQuiet) {
    char* argv[] = {(char*)"lulesh", (char*)"-q"};
    int argc = 2;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.quiet, 1);
}

TEST_F(LuleshTest, ParseCommandLineOptionsBalance) {
    char* argv[] = {(char*)"lulesh", (char*)"-b", (char*)"2"};
    int argc = 3;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.balance, 2);
}

TEST_F(LuleshTest, ParseCommandLineOptionsCost) {
    char* argv[] = {(char*)"lulesh", (char*)"-c", (char*)"3"};
    int argc = 3;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.cost, 3);
}

TEST_F(LuleshTest, ParseCommandLineOptionsMultiple) {
    char* argv[] = {(char*)"lulesh", (char*)"-i", (char*)"100", (char*)"-s", (char*)"50", (char*)"-q"};
    int argc = 6;
    cmdLineOpts opts;
    memset(&opts, 0, sizeof(opts));
    
    ParseCommandLineOptions(argc, argv, 0, &opts);
    
    EXPECT_EQ(opts.its, 100);
    EXPECT_EQ(opts.nx, 50);
    EXPECT_EQ(opts.quiet, 1);
}

TEST_F(LuleshTest, DomainConstructorDestructor) {
    Int_t numRanks = 1;
    Index_t colLoc = 0;
    Index_t rowLoc = 0;
    Index_t planeLoc = 0;
    Index_t nx = 10;
    Int_t tp = 1;
    Int_t nr = 11;
    Int_t balance = 1;
    Int_t cost = 1;
    
    Domain* domain = new Domain(numRanks, colLoc, rowLoc, planeLoc, nx, tp, nr, balance, cost);
    ASSERT_NE(domain, nullptr);
    
    EXPECT_EQ(domain->numRanks(), numRanks);
    EXPECT_EQ(domain->colLoc(), colLoc);
    EXPECT_EQ(domain->rowLoc(), rowLoc);
    EXPECT_EQ(domain->planeLoc(), planeLoc);
    EXPECT_EQ(domain->sizeX(), nx);
    EXPECT_EQ(domain->numReg(), nr);
    EXPECT_EQ(domain->cost(), cost);
    
    delete domain;
}

TEST_F(LuleshTest, DomainNodeAccessors) {
    Int_t numRanks = 1;
    Index_t colLoc = 0;
    Index_t rowLoc = 0;
    Index_t planeLoc = 0;
    Index_t nx = 3;
    Int_t tp = 1;
    Int_t nr = 11;
    Int_t balance = 1;
    Int_t cost = 1;
    
    Domain* domain = new Domain(numRanks, colLoc, rowLoc, planeLoc, nx, tp, nr, balance, cost);
    ASSERT_NE(domain, nullptr);
    
    Index_t idx = 0;
    domain->x(idx) = 1.0;
    domain->y(idx) = 2.0;
    domain->z(idx) = 3.0;
    
    EXPECT_DOUBLE_EQ(domain->x(idx), 1.0);
    EXPECT_DOUBLE_EQ(domain->y(idx), 2.0);
    EXPECT_DOUBLE_EQ(domain->z(idx), 3.0);
    
    domain->xd(idx) = 4.0;
    domain->yd(idx) = 5.0;
    domain->zd(idx) = 6.0;
    
    EXPECT_DOUBLE_EQ(domain->xd(idx), 4.0);
    EXPECT_DOUBLE_EQ(domain->yd(idx), 5.0);
    EXPECT_DOUBLE_EQ(domain->zd(idx), 6.0);
    
    domain->xdd(idx) = 7.0;
    domain->ydd(idx) = 8.0;
    domain->zdd(idx) = 9.0;
    
    EXPECT_DOUBLE_EQ(domain->xdd(idx), 7.0);
    EXPECT_DOUBLE_EQ(domain->ydd(idx), 8.0);
    EXPECT_DOUBLE_EQ(domain->zdd(idx), 9.0);
    
    domain->fx(idx) = 10.0;
    domain->fy(idx) = 11.0;
    domain->fz(idx) = 12.0;
    
    EXPECT_DOUBLE_EQ(domain->fx(idx), 10.0);
    EXPECT_DOUBLE_EQ(domain->fy(idx), 11.0);
    EXPECT_DOUBLE_EQ(domain->fz(idx), 12.0);
    
    domain->nodalMass(idx) = 13.0;
    
    EXPECT_DOUBLE_EQ(domain->nodalMass(idx), 13.0);
    
    delete domain;
}

TEST_F(LuleshTest, DomainElemAccessors) {
    Int_t numRanks = 1;
    Index_t colLoc = 0;
    Index_t rowLoc = 0;
    Index_t planeLoc = 0;
    Index_t nx = 3;
    Int_t tp = 1;
    Int_t nr = 11;
    Int_t balance = 1;
    Int_t cost = 1;
    
    Domain* domain = new Domain(numRanks, colLoc, rowLoc, planeLoc, nx, tp, nr, balance, cost);
    ASSERT_NE(domain, nullptr);
    
    Index_t idx = 0;
    domain->e(idx) = 1.0;
    domain->p(idx) = 2.0;
    domain->q(idx) = 3.0;
    
    EXPECT_DOUBLE_EQ(domain->e(idx), 1.0);
    EXPECT_DOUBLE_EQ(domain->p(idx), 2.0);
    EXPECT_DOUBLE_EQ(domain->q(idx), 3.0);
    
    domain->ql(idx) = 4.0;
    domain->qq(idx) = 5.0;
    
    EXPECT_DOUBLE_EQ(domain->ql(idx), 4.0);
    EXPECT_DOUBLE_EQ(domain->qq(idx), 5.0);
    
    domain->v(idx) = 6.0;
    domain->volo(idx) = 7.0;
    domain->delv(idx) = 8.0;
    domain->vdov(idx) = 9.0;
    
    EXPECT_DOUBLE_EQ(domain->v(idx), 6.0);
    EXPECT_DOUBLE_EQ(domain->volo(idx), 7.0);
    EXPECT_DOUBLE_EQ(domain->delv(idx), 8.0);
    EXPECT_DOUBLE_EQ(domain->vdov(idx), 9.0);
    
    domain->arealg(idx) = 10.0;
    domain->ss(idx) = 11.0;
    domain->elemMass(idx) = 12.0;
    
    EXPECT_DOUBLE_EQ(domain->arealg(idx), 10.0);
    EXPECT_DOUBLE_EQ(domain->ss(idx), 11.0);
    EXPECT_DOUBLE_EQ(domain->elemMass(idx), 12.0);
    
    delete domain;
}

TEST_F(LuleshTest, DomainParameterAccessors) {
    Int_t numRanks = 1;
    Index_t colLoc = 0;
    Index_t rowLoc = 0;
    Index_t planeLoc = 0;
    Index_t nx = 3;
    Int_t tp = 1;
    Int_t nr = 11;
    Int_t balance = 1;
    Int_t cost = 1;
    
    Domain* domain = new Domain(numRanks, colLoc, rowLoc, planeLoc, nx, tp, nr, balance, cost);
    ASSERT_NE(domain, nullptr);
    
    EXPECT_GT(domain->u_cut(), 0.0);
    EXPECT_GT(domain->e_cut(), 0.0);
    EXPECT_GT(domain->p_cut(), 0.0);
    EXPECT_GT(domain->q_cut(), 0.0);
    EXPECT_GT(domain->v_cut(), 0.0);
    
    EXPECT_GT(domain->hgcoef(), 0.0);
    EXPECT_GT(domain->ss4o3(), 0.0);
    EXPECT_GT(domain->qstop(), 0.0);
    
    domain->time() = 1.0;
    domain->deltatime() = 0.1;
    domain->stoptime() = 10.0;
    
    EXPECT_DOUBLE_EQ(domain->time(), 1.0);
    EXPECT_DOUBLE_EQ(domain->deltatime(), 0.1);
    EXPECT_DOUBLE_EQ(domain->stoptime(), 10.0);
    
    domain->cycle() = 5;
    
    EXPECT_EQ(domain->cycle(), 5);
    
    delete domain;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}