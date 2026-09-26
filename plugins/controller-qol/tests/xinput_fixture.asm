.code
ALIGN 16
XInputFixture PROC
 db 048h,089h,05ch,024h,008h
 mov eax,123
 ret
XInputFixture ENDP
ALIGN 16
XInputFixturePrior PROC
 db 048h,089h,05ch,024h,008h
 mov eax,123
 ret
XInputFixturePrior ENDP
END
