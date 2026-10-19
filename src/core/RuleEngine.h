//
// Created by Birka on 18/10/2026.
//

#ifndef RULEENGINE_H
#define RULEENGINE_H
#include <utility>
#include <vector>

#include "FileEvent.h"
#include "Rule.h"

class RuleEngine {
public:
    explicit RuleEngine(std::vector<Rule> rules) : m_rules(std::move(rules)) {}
    std::expected<bool, ActionError> executeActions(const FileReady& file) const;
private:
    std::vector<Rule> m_rules;
};



#endif //RULEENGINE_H
