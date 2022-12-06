! Fortran version of test.cpp

program fort_test
  use meric

  call MERIC_Init()

  call MERIC_MeasureStart("A"//char(0))

  call MERIC_MeasureStart("B"//char(0))
  call sleep(1)
  call MERIC_MeasureStop("B"//char(0))

  call MERIC_MeasureStart("C"//char(0))
  call sleep(1)
  call MERIC_MeasureStop("C"//char(0))

  call MERIC_MeasureStart("B"//char(0))
  call sleep(2)
  call MERIC_MeasureStop("B"//char(0))

  call MERIC_MeasureStop("A"//char(0))

  call MERIC_Close()

end program fort_test

