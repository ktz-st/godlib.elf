        section data
        even
        xdef CutBuild_Source,CutBuild_SourceEnd,CutBuild_Binary,CutBuild_BinaryEnd
CutBuild_Source: incbin 'fixture.txt'
CutBuild_SourceEnd: dc.b 0
        even
CutBuild_Binary: incbin 'fixture.cut'
CutBuild_BinaryEnd:
        even
