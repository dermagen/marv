;; =============================================================================
;; test/test.scm -- Unit & RV64G Simulation Test Suite for MARV
;; =============================================================================

(import (scheme base)
        (scheme read)
        (scheme write)
        (scheme file)
        (scheme process-context)
        (skint)
        (skint hidden))

(include "../src/skint/marv.scm")
(include "rv64-sim.scm")

(define *tests-passed* 0)
(define *tests-failed* 0)

(define (assert-eq label actual expected)
  (if (equal? actual expected)
      (begin
        (set! *tests-passed* (+ *tests-passed* 1))
        (printf "  [PASS] ~a = ~a~%" label actual))
      (begin
        (set! *tests-failed* (+ *tests-failed* 1))
        (printf "  [FAIL] ~a: expected ~a, got ~a~%" label expected actual))))

(define (string-contains? str sub)
  (let ((slen (string-length str))
        (blen (string-length sub)))
    (let loop ((i 0))
      (cond ((> (+ i blen) slen) #f)
            ((string=? (substring str i (+ i blen)) sub) #t)
            (else (loop (+ i 1)))))))

(printf "=== MARV Standard Library & Compiler Unit Suite ===~%")

;; 1. Verify that all 13 `.mvh` system headers parse and expand cleanly
(let* ((test-forms
         '((include system $ctype)
           (include system $errno)
           (include system $fenv)
           (include system $math)
           (include system $stdio)
           (include system $stdlib)
           (include system $string)
           (include system $time)
           (include system $unistd)
           (include system $fcntl)
           (include system $dirent)
           (include system $sys.stat)
           (include system $math.vec)
           (module $all_hdrs_test
             (func $check (export "check") (result i32)
               (return
                 (i32.add
                   (const $O_CREAT)
                   (i32.add (const $DT_DIR)
                            (i32.add (const $EDOM)
                                     (i32.add (sizeof $stat)
                                              (i32.add (sizeof $tm)
                                                       (sizeof $vec3)))))))))))
       (expanded (expand-mvs-forms test-forms "."))
       (mod (parse-module expanded))
       (wo-str (let ((p (open-output-string))) (emit-wo mod p) (get-output-string p))))
  ;; O_CREAT=4096 + DT_DIR=3 + EDOM=18 + sizeof(stat)=80 + sizeof(tm)=36 + sizeof(vec3)=24 = 4257
  (assert-eq "All 13 .mvh headers parse & expand constants + record sizes"
             (and (string-contains? wo-str "i32.const 4096")
                  (string-contains? wo-str "i32.const 80")
                  (string-contains? wo-str "i32.const 36")
                  (string-contains? wo-str "i32.const 24"))
             #t))

;; 2. Verify code emission for `examples/dirent_demo.mvs`
(let* ((wo-str (let ((p (open-output-string)))
                 (compile-marv-file "examples/dirent_demo.mvs" 'wo p)
                 (get-output-string p)))
       (c-str  (let ((p (open-output-string)))
                 (compile-marv-file "examples/dirent_demo.mvs" 'c99 p)
                 (get-output-string p)))
       (rv-str (let ((p (open-output-string)))
                 (compile-marv-file "examples/dirent_demo.mvs" 'rv64g p)
                 (get-output-string p))))
  (assert-eq ".wo imports $dirent:opendir directly"
             (string-contains? wo-str "(import \"dirent\" \"opendir\" (func $dirent:opendir") #t)
  (assert-eq ".wo omits unreferenced $dirent:fdopendir"
             (string-contains? wo-str "fdopendir") #f)
  (assert-eq ".wo passes internal MARV callback via direct ref.func"
             (string-contains? wo-str "ref.func $dirent_demo:filter_mvs_or_dir") #t)
  (assert-eq ".wo passes imported $alphasort via direct ref.func $dirent:alphasort"
             (string-contains? wo-str "ref.func $dirent:alphasort") #t)
  (assert-eq "CC calls dirent_opendir via <mod>_<name> MARV-ABI convention"
             (string-contains? c-str "dirent_opendir(") #t))

;; 3. Verify `examples/libc_demo.mvs` (.wo & CC emission for varargs, records, globals, data)
(let* ((wo-str (let ((p (open-output-string)))
                 (compile-marv-file "examples/libc_demo.mvs" 'wo p)
                 (get-output-string p)))
       (c-str  (let ((p (open-output-string)))
                 (compile-marv-file "examples/libc_demo.mvs" 'c99 p)
                 (get-output-string p))))
  (assert-eq ".wo imports variadic $stdio:snprintf with trailing ptr ($ap$)"
             (string-contains? wo-str "(import \"stdio\" \"snprintf\" (func $stdio:snprintf (param i32) (param i32) (param i32) (param i32) (result i32)))") #t)
  (assert-eq ".wo imports record-returning $stdlib:div with leading ptr ($rp$)"
             (string-contains? wo-str "(import \"stdlib\" \"div\" (func $stdlib:div (param i32) (param i32) (param i32)))") #t)
  (assert-eq ".wo imports nested module $math.vec:cross with leading ptr ($rp$)"
             (string-contains? wo-str "(import \"math.vec\" \"cross\" (func $math.vec:cross (param i32) (param i32) (param i32)))") #t)
  (assert-eq ".wo imports $stdio:_iob data table directly"
             (string-contains? wo-str "(import \"stdio\" \"_iob\" (data $stdio:_iob") #t)
  (assert-eq ".wo imports $errno:errno global directly"
             (string-contains? wo-str "(import \"errno\" \"errno\" (global $errno:errno (mut i32)))") #t)
  (assert-eq "CC resolves nested module math.vec to math_vec_cross"
             (string-contains? c-str "math_vec_cross(") #t))

;; 4. Verify RV64G assembly execution of `dirent_demo.s` in `rv64-sim.scm`
(let ((rv-file "/tmp/marv_test_dirent.s"))
  (when (file-exists? rv-file) (delete-file rv-file))
  (call-with-output-file rv-file
    (lambda (p) (compile-marv-file "examples/dirent_demo.mvs" 'rv64g p)))
  (let ((sim (rv64-load-asm rv-file)))
    (assert-eq "RV64G dirent_demo: test_stream_ops()"
               (rv64-call sim "test_stream_ops" '()) 100)
    (assert-eq "RV64G dirent_demo: test_scandir_ops(0)"
               (rv64-call sim "test_scandir_ops" '(0)) 200)
    (assert-eq "RV64G dirent_demo: main(1, 0)"
               (rv64-call sim "main" '(1 0)) 0)))

;; 5. Verify RV64G assembly execution of `libc_demo.s` in `rv64-sim.scm`
(let ((rv-file "/tmp/marv_test_libc.s"))
  (when (file-exists? rv-file) (delete-file rv-file))
  (call-with-output-file rv-file
    (lambda (p) (compile-marv-file "examples/libc_demo.mvs" 'rv64g p)))
  (let ((sim (rv64-load-asm rv-file)))
    (assert-eq "RV64G libc_demo: test_qsort_bsearch()"
               (rv64-call sim "test_qsort_bsearch" '()) 1000)
    (assert-eq "RV64G libc_demo: test_snprintf_varargs() [snprintf + user va.arg]"
               (rv64-call sim "test_snprintf_varargs" '()) 3027)
    (assert-eq "RV64G libc_demo: test_records_and_nested_mod() [div + math.vec + sys.stat]"
               (rv64-call sim "test_records_and_nested_mod" '()) 3418)
    (assert-eq "RV64G libc_demo: test_libc_misc() [errno + strtoll + strtod + ctype + stdout]"
               (rv64-call sim "test_libc_misc" '()) 500)
    (assert-eq "RV64G libc_demo: run_all_mstd_tests()"
               (rv64-call sim "run_all_mstd_tests" '()) 7945)
    (assert-eq "RV64G libc_demo: main(1, 0)"
               (rv64-call sim "main" '(1 0)) 0)))

;; 6. Verify RV64G assembly execution of `queens.s` in `rv64-sim.scm`
(let ((rv-file "/tmp/marv_test_queens.s"))
  (when (file-exists? rv-file) (delete-file rv-file))
  (call-with-output-file rv-file
    (lambda (p) (compile-marv-file "examples/queens.mvs" 'rv64g p)))
  (let ((sim (rv64-load-asm rv-file)))
    (assert-eq "RV64G queens: solve_eight_queens(0)"
               (rv64-call sim "solve_eight_queens" '(0)) 92)
    (assert-eq "RV64G queens: bounded O(1) tail-call stack usage (<= 512 bytes)"
               (<= *rv64-last-max-stack-bytes* 512) #t)
    (assert-eq "RV64G queens: solve_eight_queens(1)"
               (rv64-call sim "solve_eight_queens" '(1)) 92)
    (assert-eq "RV64G queens: main(1, 0)"
               (rv64-call sim "main" '(1 0)) 0)))

;; 7. Verify 3-module separate compilation (`vec3.mvs`, `stats.mvs`, `app.mvs`)
(let ((vec3-s  "/tmp/marv_mm_vec3.s")
      (stats-s "/tmp/marv_mm_stats.s")
      (app-s   "/tmp/marv_mm_app.s")
      (vec3-wo (let ((p (open-output-string)))
                 (compile-marv-file "examples/multimod/vec3.mvs" 'wo p)
                 (get-output-string p)))
      (stats-wo (let ((p (open-output-string)))
                  (compile-marv-file "examples/multimod/stats.mvs" 'wo p)
                  (get-output-string p)))
      (app-wo  (let ((p (open-output-string)))
                 (compile-marv-file "examples/multimod/app.mvs" 'wo p)
                 (get-output-string p))))
  (assert-eq "vec3.wo (including vec3.mvh) does not self-import $vec3:dot"
             (string-contains? vec3-wo "(import \"vec3\"") #f)
  (assert-eq "stats.wo (including stats.mvh + vec3.mvh) imports $vec3:make with $rp$"
             (string-contains? stats-wo "(import \"vec3\" \"make\" (func $vec3:make (param i32) (param f64) (param f64) (param f64)))") #t)
  (assert-eq "app.wo imports variadic $stats:weighted_sum_va with trailing $ap$"
             (string-contains? app-wo "(import \"stats\" \"weighted_sum_va\" (func $stats:weighted_sum_va (param i32) (param i32) (result i32)))") #t)
  (assert-eq "app.wo imports cross-module global $vec3:ops_count"
             (string-contains? app-wo "(import \"vec3\" \"ops_count\" (global $vec3:ops_count (mut i32)))") #t)
  (assert-eq "app.wo imports cross-module data $stats:tag_label"
             (string-contains? app-wo "(import \"stats\" \"tag_label\" (data $stats:tag_label const align=8 size=8))") #t)

  (for-each (lambda (f) (when (file-exists? f) (delete-file f)))
            (list vec3-s stats-s app-s))
  (call-with-output-file vec3-s
    (lambda (p) (compile-marv-file "examples/multimod/vec3.mvs" 'rv64g p)))
  (call-with-output-file stats-s
    (lambda (p) (compile-marv-file "examples/multimod/stats.mvs" 'rv64g p)))
  (call-with-output-file app-s
    (lambda (p) (compile-marv-file "examples/multimod/app.mvs" 'rv64g p)))

  (let ((sim (rv64-load-asm (list vec3-s stats-s app-s))))
    (assert-eq "RV64G 3-module linked: stats_weighted_sum_va via app_run_multimod_suite(0)"
               (rv64-call sim "app_run_multimod_suite" '(0)) 4200)
    (assert-eq "RV64G 3-module linked: main(1, 0)"
               (rv64-call sim "main" '(1 0)) 0)))

;; 8. Verify CLI lookup directory defaults, .mvt target loading, and system .c adapter resolution
(assert-eq "Default *mstd-target-dirs* is (\"target/\" \"targets/\")"
           *mstd-target-dirs* '("target/" "targets/"))
(assert-eq "Default *mstd-system-dirs* is (\"lib/\" \"libs/\")"
           *mstd-system-dirs* '("lib/" "libs/"))
(assert-eq "Default *mstd-local-dirs* is (\"./\")"
           *mstd-local-dirs* '("./"))

(let ((wcpl-mvt (load-mvt-file (find-mvt-file "wcpl")))
      (cc-mvt   (load-mvt-file (find-mvt-file "cc")))
      (rv-mvt   (load-mvt-file (find-mvt-file "rv64g"))))
  (assert-eq "wcpl.mvt resolves target type wcpl and .wo extension"
             (list (cdr (assq 'type wcpl-mvt)) (cdr (assq 'obj-ext wcpl-mvt)))
             '(wcpl ".wo"))
  (assert-eq "cc.mvt resolves target type cc and .c extension"
             (list (cdr (assq 'type cc-mvt)) (cdr (assq 'obj-ext cc-mvt)))
             '(cc ".c"))
  (assert-eq "rv64g.mvt resolves target type rv64g and .s extension"
             (list (cdr (assq 'type rv-mvt)) (cdr (assq 'obj-ext rv-mvt)))
             '(rv64g ".s")))

(assert-eq "mvh-path->adapter-c-path maps lib/dirent.mvh to lib/dirent.c"
           (mvh-path->adapter-c-path "lib/dirent.mvh")
           "lib/dirent.c")
(assert-eq "mvh-path->adapter-c-path maps lib/math.vec.mvh to lib/math.vec.c"
           (mvh-path->adapter-c-path "lib/math.vec.mvh")
           "lib/math.vec.c")

(printf "=== Summary: ~a passed, ~a failed ===~%" *tests-passed* *tests-failed*)
(when (> *tests-failed* 0) (exit 1))
