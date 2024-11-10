name = "Big Bucket"
class = "character" -- this might end up being a constant instead?
flavor = "The kind that makes you go “woah, that's a really big bucket.”"
subtype = { { "Good", 1.0 } }
aa = 3
ad = 0
dd = 6
abilities = {
    {
        name = "Anti-Grease Bucket Beam",
        description = "Launch a devastating beam attack against one target " ..
        "character card, destroying it if it's attack is 3 or less (+2 if " ..
        "the character is of the Grease subtype, weighted.)",
        action = function (target)
            local grease = get_subtype(target, "Grease")
            local damage = 3 + 2 * grease
            local target_aa = get_aa(target)
            if target_aa <= damage then
                destroy(target)
            end
        end,
        action_type = "targeted",
        action_target = function (target)
            local class = get_class(target)
            return class == "character"
        end
    },
    {
        name = "fireball"
    }
}
