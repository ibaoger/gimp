;; -*-scheme-*-

;; Alan Horkan 2004.  Public Domain.
;; so long as remove this block of comments from your script
;; feel free to use it for whatever you like.

(define (script-fu-guide-new image
                             drawables
                             direction
                             position)
  (let* (
        (width (car (gimp-image-get-width image)))
        (height (car (gimp-image-get-height image)))
        )

    (if (= direction 0)
        ;; check position is inside the image boundaries
        (if (<= position height) (gimp-image-add-hguide image position))
        (if (<= position width) (gimp-image-add-vguide image position))
    )

    (gimp-displays-flush)
  )
)

(script-fu-register-filter "script-fu-guide-new"
  _"New _Guide..."
  _"Add a guide at the orientation and position specified (in pixels)"
  "Alan Horkan"
  "Alan Horkan, 2004.  Public Domain."
  "2004-04-02"
  "*"
  SF-ONE-OR-MORE-DRAWABLE  ; doesn't matter how many drawables are selected
  SF-OPTION     _"_Direction" '(_"Horizontal" _"Vertical")
  SF-ADJUSTMENT _"_Position"  (list 0 0 MAX-IMAGE-SIZE 1 10 0 1)
)

(script-fu-menu-register "script-fu-guide-new"
                         "<Image>/Image/Guides")
