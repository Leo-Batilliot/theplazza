/*
** EPITECH PROJECT, 2026
** plazza
** File description:
** Bootstrap smoke test - proves the test pipeline links and runs.
** Real suites land with PLZ-008 onwards.
*/

#include <criterion/criterion.h>

Test(bootstrap, build_pipeline_works)
{
    cr_assert(1, "Criterion linkage and runner OK");
}
