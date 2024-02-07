/**
 ******************************************************************************
 * @file    nECU_table.c
 * @brief   This file provides code for interpolated tables.
 *          Those tables work the same as regular tuning tables.
 ******************************************************************************
 */

#include "nECU_table.h"

/* Interpolated reference table */
void nECU_Table_Set(Knock_Interpol_Table *Table, const float *Axis, const float *Values1, const float *Values2, uint32_t length) // fill table with constants
{
    Table->size = length;
    for (uint8_t i = 1; i < length + 1; i++)
    {
        Table->Table[i][1] = Axis[i - 1];
        Table->Table[i][2] = Values1[i - 1];
        Table->Table[i][3] = Values2[i - 1];
    }
    // first and last value set to properly interpolate
    Table->Table[0][1] = 0;
    Table->Table[0][2] = Values1[0];
    Table->Table[0][3] = Values2[0];
    Table->Table[length + 1][1] = 999999999;
    Table->Table[length + 1][2] = Values1[length - 1];
    Table->Table[length + 1][3] = Values2[length - 1];
}
void nECU_Table_Get(float *input, Knock_Interpol_Table *Table, float *Out1, float *Out2) // find value corresponding to table
{
    /* find index */
    uint8_t index = 1;
    for (uint8_t i = 1; i < FFT_THRESH_TABLE_LEN; i++)
    {
        if (Table->Table[i][1] <= *input)
        {
            index++;
        }
        else
        {
            break;
        }
    }
    *Out1 = nECU_Table_Interpolate(&(Table->Table[index][1]), &(Table->Table[index][2]), &(Table->Table[index + 1][1]), &(Table->Table[index + 1][2]), input);
    *Out2 = nECU_Table_Interpolate(&(Table->Table[index][1]), &(Table->Table[index][3]), &(Table->Table[index + 1][1]), &(Table->Table[index + 1][3]), input);
}
float nECU_Table_Interpolate(float *Ax, float *Ay, float *Bx, float *By, float *X) // function to interpolate linearly between two points A&B where
{
    if (*X > *Bx)
    {
        return *By;
    }
    else if (*X < *Ax)
    {
        return *Ay;
    }
    else
    {
        float Y = 0;
        Y = *Ay * (*Bx - *X);
        Y += *By * (*X - *Ax);
        Y /= (*Bx - *Ax);
        return Y;
    }
}
bool nECU_Table_Interpolate_Test(void) // test interpolation method
{
    float Ax = 1000, Ay = 1, Bx = 2000, By = 2, X = 1200, Y = 1.2;
    float result = nECU_Table_Interpolate(&Ax, &Ay, &Bx, &By, &X);
    if (Y - result)
    {
        return true;
    }
    return false;
}
