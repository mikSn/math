/*
 * vector_expressions.hpp
 *
 *  Created on: Dec 25, 2018
 *      Author: ser-fedorov
 */

#ifndef PUSHKIN_MATH_DETAIL_VECTOR_EXPRESSIONS_HPP_
#define PUSHKIN_MATH_DETAIL_VECTOR_EXPRESSIONS_HPP_

#include <pushkin/math/detail/expressions.hpp>
#include <pushkin/math/detail/scalar_expressions.hpp>

#include <stdexcept>

namespace psst {
namespace math {
namespace expr {

inline namespace v {

//----------------------------------------------------------------------------
template <typename Expression, typename Result = Expression>
struct vector_expression {
    static_assert(is_vector_v<Result>, "Result of vector expression must be a vector");
    using expression_type     = Expression;
    using result_type         = Result;
    using traits              = value_traits_t<result_type>;
    using value_type          = typename traits::value_type;
    using value_tag           = typename traits::value_tag;
    using axes_names          = typename traits::axes_names;
    using index_sequence_type = typename traits::index_sequence_type;

    static constexpr auto size = traits::size;

    template <std::size_t N>
    constexpr value_type
    at() const
    {
        static_assert(N < size, "Invalid vector expression index");
        return rebind().template at<N>();
    }
    constexpr result_type
    value() const
    {
        return result_type(rebind());
    }

    constexpr operator expression_type const&() const { return rebind(); }

private:
    constexpr expression_type const&
    rebind() const
    {
        return static_cast<expression_type const&>(*this);
    }
};

template <template <typename, typename> class Expression, typename LHS, typename RHS,
          typename Result = vector_expression_result_t<LHS, RHS>>
using binary_vector_expression = vector_expression<Expression<LHS, RHS>, Result>;

template <std::size_t N, typename Expression, typename Result>
constexpr auto
get(vector_expression<Expression, Result> const& exp)
{
    return exp.template at<N>();
}

template <typename Expression, typename Result>
constexpr std::size_t
size_of(vector_expression<Expression, Result> const&)
{
    return vector_expression<Expression, Result>::size;
}

template <typename Vector>
struct vector_fill
    : vector_expression<vector_fill<Vector>, typename std::decay_t<Vector>::result_type> {
    static_assert(is_vector_expression_v<Vector>, "Template argument must be a vector expression");
    using base_type
        = vector_expression<vector_fill<Vector>, typename std::decay_t<Vector>::result_type>;
    using value_type = typename base_type::value_type;

    vector_fill(value_type v) : arg_{v} {}

    template <std::size_t N>
    constexpr value_type
    at() const
    {
        return arg_;
    }

private:
    value_type arg_;
};

namespace detail {

template <typename T>
constexpr std::size_t
vector_expression_size()
{
    static_assert(is_vector_expression_v<T>, "The exression is not vector type");
    return std::decay_t<T>::size;
}

}    // namespace detail

template <typename T>
struct vector_expression_size
    : std::integral_constant<std::size_t, detail::vector_expression_size<T>()> {};
template <typename T>
using vector_exression_size_t = typename vector_expression_size<T>::type;
template <typename T>
constexpr std::size_t vector_expression_size_v = vector_exression_size_t<T>::value;

//----------------------------------------------------------------------------
//@{
/** @name Compare vector expressions */
namespace detail {

template <std::size_t N, typename LHS, typename RHS>
struct vector_expression_cmp;

template <typename LHS, typename RHS>
struct vector_expression_cmp<0, LHS, RHS> {
    static_assert((is_vector_expression_v<LHS> && is_vector_expression_v<RHS>),
                  "Both sides to the comparison must be vector expressions");
    using lhs_type    = std::decay_t<LHS>;
    using rhs_type    = std::decay_t<RHS>;
    using traits_type = value_traits_t<typename lhs_type::value_type>;

    constexpr static int
    cmp(lhs_type const& lhs, rhs_type const& rhs)
    {
        return traits_type::cmp(lhs.template at<0>(), rhs.template at<0>());
    }
};

template <std::size_t N, typename LHS, typename RHS>
struct vector_expression_cmp {
    static_assert((is_vector_expression_v<LHS> && is_vector_expression_v<RHS>),
                  "Both sides to the comparison must be vector expressions");
    using prev_element = vector_expression_cmp<N - 1, LHS, RHS>;
    using lhs_type     = typename prev_element::lhs_type;
    using rhs_type     = typename prev_element::rhs_type;
    using traits_type  = typename prev_element::traits_type;
    constexpr static int
    cmp(lhs_type const& lhs, rhs_type const& rhs)
    {
        auto res = prev_element::cmp(lhs, rhs);
        if (res == 0) {
            return traits_type::cmp(lhs.template at<N>(), rhs.template at<N>());
        }
        return res;
    }
};

}    // namespace detail

template <typename LHS, typename RHS>
struct vector_cmp : binary_scalar_expression<vector_cmp, LHS, RHS, int>,
                    binary_expression<LHS, RHS> {

    static_assert((is_vector_expression_v<LHS> && is_vector_expression_v<RHS>),
                  "Both sides to the comparison must be vector expressions");
    static constexpr std::size_t cmp_size
        = utils::min_v<vector_expression_size_v<LHS>, vector_expression_size_v<RHS>>;
    using cmp_type = detail::vector_expression_cmp<cmp_size - 1, LHS, RHS>;

    using expression_base = binary_expression<LHS, RHS>;
    using expression_base::expression_base;

    constexpr int
    value() const
    {
        return cmp_type::cmp(this->lhs_, this->rhs_);
    }
};
template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr int
cmp(LHS&& lhs, RHS&& rhs)
{
    return make_binary_expression<vector_cmp>(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}
//@}

//----------------------------------------------------------------------------
//@{
/** @name Check two vector expressions equality */
template <typename LHS, typename RHS>
struct vector_eq : binary_scalar_expression<vector_eq, LHS, RHS, bool>,
                   binary_expression<LHS, RHS> {
    static_assert((is_vector_expression_v<LHS> && is_vector_expression_v<RHS>),
                  "Both sides to the comparison must be vector expressions");
    static constexpr std::size_t cmp_size
        = utils::min_v<vector_expression_size_v<LHS>, vector_expression_size_v<RHS>>;
    using cmp_type = detail::vector_expression_cmp<cmp_size - 1, LHS, RHS>;

    using expression_base = binary_expression<LHS, RHS>;
    using expression_base::expression_base;

    constexpr bool
    value() const
    {
        return cmp_type::cmp(this->lhs_, this->rhs_) == 0;
    }
};

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
operator==(LHS&& lhs, RHS&& rhs)
{
    return make_binary_expression<vector_eq>(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
operator!=(LHS&& lhs, RHS&& rhs)
{
    return !(std::forward<LHS>(lhs) == std::forward<RHS>(rhs));
}
//@}

//----------------------------------------------------------------------------
//@{
/** @name Vector comparison */
template <typename LHS, typename RHS>
struct vector_less : binary_scalar_expression<vector_less, LHS, RHS, bool>,
                     binary_expression<LHS, RHS> {
    static_assert((is_vector_expression_v<LHS> && is_vector_expression_v<RHS>),
                  "Both sides to the comparison must be vector expressions");
    static constexpr std::size_t cmp_size
        = utils::min_v<vector_expression_size_v<LHS>, vector_expression_size_v<RHS>>;
    using cmp_type = detail::vector_expression_cmp<cmp_size - 1, LHS, RHS>;

    using expression_base = binary_expression<LHS, RHS>;
    using expression_base::expression_base;

    constexpr bool
    value() const
    {
        return cmp_type::cmp(this->lhs_, this->rhs_) < 0;
    }
};

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
operator<(LHS&& lhs, RHS&& rhs)
{
    return make_binary_expression<vector_less>(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
operator<=(LHS&& lhs, RHS&& rhs)
{
    return !(std::forward<RHS>(rhs) < std::forward<LHS>(lhs));
}

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
operator>(LHS&& lhs, RHS&& rhs)
{
    return make_binary_expression<vector_less>(std::forward<RHS>(rhs), std::forward<LHS>(lhs));
}

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
operator>=(LHS&& lhs, RHS&& rhs)
{
    return !(std::forward<LHS>(lhs) < std::forward<RHS>(rhs));
}
//@}

//----------------------------------------------------------------------------
//@{
/** @name Sum of two vectors */
template <typename LHS, typename RHS>
struct vector_sum : binary_vector_expression<vector_sum, LHS, RHS>, binary_expression<LHS, RHS> {
    using base_type  = binary_vector_expression<vector_sum, LHS, RHS>;
    using value_type = typename base_type::value_type;

    using expression_base = binary_expression<LHS, RHS>;
    using expression_base::expression_base;

    template <std::size_t N>
    constexpr value_type
    at() const
    {
        static_assert(N < base_type::size, "Vector sum element index is out of range");
        return this->lhs_.template at<N>() + this->rhs_.template at<N>();
    }
};

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
operator+(LHS&& lhs, RHS&& rhs)
{
    return make_binary_expression<vector_sum>(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}
//@}

//----------------------------------------------------------------------------
//@{
/** @name Substraction of two vectors */
template <typename LHS, typename RHS>
struct vector_diff : binary_vector_expression<vector_diff, LHS, RHS>, binary_expression<LHS, RHS> {
    using base_type  = binary_vector_expression<vector_diff, LHS, RHS>;
    using value_type = typename base_type::value_type;

    using expression_base = binary_expression<LHS, RHS>;
    using expression_base::expression_base;

    template <std::size_t N>
    constexpr value_type
    at() const
    {
        static_assert(N < base_type::size, "Vector difference element index is out of range");
        return this->lhs_.template at<N>() - this->rhs_.template at<N>();
    }
};

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
operator-(LHS&& lhs, RHS&& rhs)
{
    return make_binary_expression<vector_diff>(std::forward<LHS>(lhs), std::forward<RHS>(rhs));
}
//@}

//----------------------------------------------------------------------------
//@{
/** @name Multiplication of a vector by scalar expression */
template <typename LHS, typename RHS>
struct vector_scalar_multiply : binary_vector_expression<vector_scalar_multiply, LHS, RHS>,
                                binary_expression<LHS, RHS> {
    using base_type  = binary_vector_expression<vector_scalar_multiply, LHS, RHS>;
    using value_type = typename base_type::value_type;

    using expression_base = binary_expression<LHS, RHS>;
    using expression_base::expression_base;

    template <std::size_t N>
    constexpr value_type
    at() const
    {
        static_assert(N < base_type::size, "Vector multiply element index is out of range");
        return this->lhs_.template at<N>() * this->rhs_;
    }
};

template <typename LHS, typename RHS,
          typename = std::enable_if_t<(is_vector_expression_v<LHS> && is_scalar_v<RHS>)
                                      || (is_scalar_v<LHS> && is_vector_expression_v<RHS>)>>
constexpr auto operator*(LHS&& lhs, RHS&& rhs)
{
    if constexpr (is_vector_expression_v<LHS> && is_scalar_v<RHS>) {
        return s::detail::wrap_non_expression_args<vector_scalar_multiply>(std::forward<LHS>(lhs),
                                                                           std::forward<RHS>(rhs));
    } else if constexpr (is_scalar_v<LHS> && is_vector_expression_v<RHS>) {
        return s::detail::wrap_non_expression_args<vector_scalar_multiply>(std::forward<RHS>(rhs),
                                                                           std::forward<LHS>(lhs));
    }
}
//@}

//----------------------------------------------------------------------------
//@{
/** @name Division of a vector by a scalar expression */
template <typename LHS, typename RHS>
struct vector_scalar_divide : binary_vector_expression<vector_scalar_divide, LHS, RHS>,
                              binary_expression<LHS, RHS> {
    using base_type  = binary_vector_expression<vector_scalar_divide, LHS, RHS>;
    using value_type = typename base_type::value_type;

    using expression_base = binary_expression<LHS, RHS>;
    using expression_base::expression_base;

    template <std::size_t N>
    constexpr value_type
    at() const
    {
        static_assert(N < base_type::size, "Vector divide element index is out of range");
        return this->lhs_.template at<N>() / this->rhs_;
    }
};
template <typename LHS, typename RHS,
          typename = std::enable_if_t<is_vector_expression_v<LHS> && is_scalar_v<RHS>>>
constexpr auto
operator/(LHS&& lhs, RHS&& rhs)
{
    return s::detail::wrap_non_expression_args<vector_scalar_divide>(std::forward<LHS>(lhs),
                                                                     std::forward<RHS>(rhs));
}
//@}

//----------------------------------------------------------------------------
//@{
template <typename LHS>
struct vector_element_sum : unary_scalar_expression<vector_element_sum, LHS>,
                            unary_expression<LHS> {
    using base_type  = unary_scalar_expression<vector_element_sum, LHS>;
    using value_type = typename base_type::value_type;

    using expression_base   = unary_expression<LHS>;
    using source_index_type = typename expression_base::arg_type::index_sequence_type;

    using expression_base::expression_base;

    constexpr value_type
    value() const
    {
        return sum(source_index_type{});
    }

private:
    template <std::size_t... Indexes>
    constexpr value_type
    sum(std::index_sequence<Indexes...>) const
    {
        return (get<Indexes>(this->arg_) + ...);
    }
};
//@}

//----------------------------------------------------------------------------
//@{
/** @name Vector magnitude (squared and not) */
template <typename LHS>
struct vector_magnitude_squared : unary_scalar_expression<vector_magnitude_squared, LHS>,
                                  unary_expression<LHS> {
    static_assert(is_vector_expression_v<LHS>, "Argument to magnitude must be a vector");
    using base_type  = unary_scalar_expression<vector_magnitude_squared, LHS>;
    using value_type = typename base_type::value_type;

    using expression_base   = unary_expression<LHS>;
    using source_index_type = typename std::decay_t<LHS>::index_sequence_type;

    using expression_base::expression_base;

    constexpr value_type
    value() const
    {
        if (value_cache_ == nval) {
            value_cache_ = sum(source_index_type{});
        }
        return value_cache_;
    }

private:
    template <std::size_t... Indexes>
    constexpr value_type
    sum(std::index_sequence<Indexes...>) const
    {
        return ((get<Indexes>(this->arg_) * get<Indexes>(this->arg_)) + ...);
    }
    // TODO Optional
    static constexpr value_type nval         = std::numeric_limits<value_type>::min();
    mutable value_type          value_cache_ = nval;
};

template <typename Expr, typename = std::enable_if_t<is_vector_expression_v<Expr>>>
constexpr auto
magnitude_square(Expr&& expr)
{
    // TODO Special handling for non-cartesian coordinate systems
    // using axes_names        = typename Expr::axes_names;
    return make_unary_expression<vector_magnitude_squared>(std::forward<Expr>(expr));
}

template <typename Expr, typename = std::enable_if_t<is_vector_expression_v<Expr>>>
constexpr auto
magnitude(Expr&& expr)
{
    // TODO Special handling for non-cartesian coordinate systems
    // using axes_names        = typename Expr::axes_names;
    return sqrt(magnitude_square(std::forward<Expr>(expr)));
}

template <typename Expr, typename = std::enable_if_t<is_vector_expression_v<Expr>>>
constexpr auto
normalize(Expr&& expr)
{
    // TODO Special handling for non-cartesian coordinate systems
    // using axes_names        = typename Expr::axes_names;
    return expr / magnitude(expr);
}

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
distance_square(LHS&& lhs, RHS&& rhs)
{
    return magnitude_square(lhs - rhs);
}

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
distance(LHS&& lhs, RHS&& rhs)
{
    return magnitude(lhs - rhs);
}
//@}

//----------------------------------------------------------------------------
//@{
/** @name Dot product of two vectors */
template <typename LHS, typename RHS>
struct vector_dot_product : binary_scalar_expression<vector_dot_product, LHS, RHS>,
                            binary_expression<LHS, RHS> {
    static_assert((is_vector_expression_v<LHS> && is_vector_expression_v<RHS>),
                  "Both sides to the dot product must be vector expressions");
    static_assert(std::decay_t<LHS>::size == std::decay_t<RHS>::size,
                  "Vector expressions must be of the same size");

    using base_type  = binary_scalar_expression<vector_dot_product, LHS, RHS>;
    using value_type = typename base_type::value_type;

    using expression_base   = binary_expression<LHS, RHS>;
    using source_index_type = typename std::decay_t<LHS>::index_sequence_type;

    using expression_base::expression_base;

    constexpr value_type
    value() const
    {
        if (value_cache_ == nval) {
            value_cache_ = sum(source_index_type{});
        }
        return value_cache_;
    }

private:
    template <std::size_t... Indexes>
    constexpr value_type
    sum(std::index_sequence<Indexes...>) const
    {
        return ((get<Indexes>(this->lhs_) * get<Indexes>(this->rhs_)) + ...);
    }
    // TODO Optional
    static constexpr value_type nval         = std::numeric_limits<value_type>::min();
    mutable value_type          value_cache_ = nval;
};

template <typename LHS, typename RHS, typename = enable_if_both_vector_expressions<LHS, RHS>>
constexpr auto
dot_product(LHS&& lhs, RHS&& rhs)
{
    return make_binary_expression<vector_dot_product>(std::forward<LHS>(lhs),
                                                      std::forward<RHS>(rhs));
}
//@}

//----------------------------------------------------------------------------
template <typename Start, typename End, typename U,
          typename = std::enable_if_t<
              is_vector_expression_v<Start> && is_vector_expression_v<End> && is_scalar_v<U>>>
constexpr auto
lerp(Start&& start, End&& end, U&& percent)
{
    return start + (end - start) * percent;
}

//----------------------------------------------------------------------------
template <typename Start, typename End, typename U,
          typename = std::enable_if_t<
              is_vector_expression_v<Start> && is_vector_expression_v<End> && is_scalar_v<U>>>
vector_expression_result_t<Start, End>
slerp(Start&& start, End&& end, U&& percent)
{
    using value_traits = typename std::decay_t<Start>::traits::value_traits;
    using std::acos;
    using std::cos;
    using std::sin;

    auto s_mag = magnitude(start);
    auto s_n   = start / s_mag;    // normalized

    auto e_mag = magnitude(end);
    auto e_n   = end / e_mag;    // normalized
    // Lerp magnitude
    auto res_mag = s_mag + (e_mag - s_mag) * percent;

    auto dot = dot_product(s_n, e_n);
    if (value_traits::eq(dot, 0)) {
        // Perpendicular vectors
        auto theta = acos(dot) * percent;
        auto res   = s_n * cos(theta) + e_n * sin(theta);
        return res * res_mag;
    } else if (value_traits::eq(dot, 1)) {
        // Collinear vectors same direction
        return lerp(start, end, percent);
    } else if (value_traits::eq(dot, -1)) {
        // Collinear vectors opposite direction
        // TODO make a vector pointing to the direction of longer vector with magnitude of 1/2 of
        // magnitude sum
        throw std::runtime_error("Slerp for opposite vectors is undefined");
    } else {
        // Generic formula
        auto omega = acos(dot);
        auto sin_o = sin(omega);
        auto res
            = (s_n * sin((1 - percent) * omega) + e_n * sin(percent * omega)) / sin_o * res_mag;
        return res;
    }
}

}    // namespace v

}    // namespace expr
}    // namespace math
}    // namespace psst

#endif /* PUSHKIN_MATH_DETAIL_VECTOR_EXPRESSIONS_HPP_ */